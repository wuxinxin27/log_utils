#ifndef LOG_UTILS_LOG_UTILS_H
#define LOG_UTILS_LOG_UTILS_H

#include <ros/ros.h>
#include <string>
#include <sstream>
#include <cstdio>
#include <fstream>
#include <memory>
#include <map>
#include <mutex>
#include <cstdlib>
#include <chrono>
#include <iomanip>

namespace log_utils {

// 日志级别枚举
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

// 获取文件名（不包含路径）
inline std::string getFileName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

// 获取当前时间戳字符串
inline std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// 日志级别转字符串
inline std::string logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// 文件日志记录器
class FileLogger {
private:
    std::string log_file_path_;
    std::ofstream log_file_;
    std::mutex mutex_;
    LogLevel min_level_;

public:
    FileLogger(const std::string& file_path, LogLevel min_level = LogLevel::DEBUG)
        : log_file_path_(file_path), min_level_(min_level) {
        log_file_.open(log_file_path_, std::ios::app);
        if (!log_file_.is_open()) {
            std::cerr << "Error: Cannot open log file: " << log_file_path_ << std::endl;
        }
    }

    ~FileLogger() {
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }

    void log(LogLevel level, const std::string& module, const std::string& file,
             int line, const std::string& message) {
        if (level < min_level_ || !log_file_.is_open()) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        log_file_ << "[" << getCurrentTimestamp() << "] "
                  << "[" << logLevelToString(level) << "] "
                  << "[" << module << "] "
                  << getFileName(file) << ":" << line << " - "
                  << message << std::endl;
        log_file_.flush();  // 确保立即写入文件
    }

    bool isOpen() const {
        return log_file_.is_open();
    }

    const std::string& getFilePath() const {
        return log_file_path_;
    }
};

// 日志管理器单例
class LogManager {
private:
    std::map<std::string, std::shared_ptr<FileLogger>> loggers_;
    std::shared_ptr<FileLogger> summary_logger_;  // 汇总日志记录器
    std::mutex mutex_;
    std::string base_log_dir_;
    bool initialized_;

    LogManager() : initialized_(false) {
        initializeLogDirectory();
    }

    void initializeLogDirectory() {
        // 尝试从环境变量获取日志目录
        const char* log_dir_env = std::getenv("LOG_DIR");
        if (log_dir_env) {
            base_log_dir_ = std::string(log_dir_env);
        } else {
            // 如果没有环境变量，使用默认路径
            const char* workspace = std::getenv("ROS_WORKSPACE");
            if (workspace) {
                base_log_dir_ = std::string(workspace) + "/logs/current";
            } else {
                base_log_dir_ = "/tmp/two_stage_int_logs";
            }
        }

        // 创建日志目录
        std::string create_dir_cmd = "mkdir -p " + base_log_dir_;
        system(create_dir_cmd.c_str());

        // 初始化汇总日志记录器
        std::string summary_log_path = base_log_dir_ + "/ALL_LOGS_SUMMARY.log";
        summary_logger_ = std::make_shared<FileLogger>(summary_log_path, LogLevel::DEBUG);

        initialized_ = true;
    }

public:
    static LogManager& getInstance() {
        static LogManager instance;
        return instance;
    }

    // 禁止拷贝和赋值
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    std::shared_ptr<FileLogger> getLogger(const std::string& module_name,
                                         LogLevel min_level = LogLevel::DEBUG) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = loggers_.find(module_name);
        if (it != loggers_.end()) {
            return it->second;
        }

        // 创建新的日志文件
        std::string log_file_path = base_log_dir_ + "/" + module_name + ".log";
        auto logger = std::make_shared<FileLogger>(log_file_path, min_level);

        if (logger->isOpen()) {
            loggers_[module_name] = logger;
            return logger;
        }

        return nullptr;
    }

    void exportLogs() {
        std::lock_guard<std::mutex> lock(mutex_);

        // 关闭所有日志文件以确保数据写入
        for (auto& pair : loggers_) {
            // FileLogger的析构函数会自动关闭文件
        }

        std::cout << "日志已导出到: " << base_log_dir_ << std::endl;

        // 列出汇总日志文件
        if (summary_logger_) {
            std::cout << "  * 汇总日志: " << summary_logger_->getFilePath() << std::endl;
        }

        // 列出所有模块日志文件
        for (const auto& pair : loggers_) {
            std::cout << "  - " << pair.second->getFilePath() << std::endl;
        }
    }

    const std::string& getLogDirectory() const {
        return base_log_dir_;
    }

    std::shared_ptr<FileLogger> getSummaryLogger() {
        return summary_logger_;
    }
};

// 自动导出器（在程序结束时自动调用）
class AutoLogExporter {
public:
    ~AutoLogExporter() {
        LogManager::getInstance().exportLogs();
    }
};

// 全局自动导出器实例
static AutoLogExporter g_auto_exporter;

// 模板函数，将任何类型转换为字符串
template<typename T>
inline std::string toString(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// 特化版本，字符串直接返回
template<>
inline std::string toString<std::string>(const std::string& value) {
    return value;
}

// 特化版本，C字符串
template<>
inline std::string toString<const char*>(const char* const& value) {
    return std::string(value);
}

// printf 风格的格式化函数
template<typename ModuleType, typename... Args>
std::string formatLogMessage(const ModuleType& module,
                           const std::string& file,
                           int line,
                           const char* format,
                           Args... args) {
    // 格式化用户消息
    char buffer[1024];
    std::snprintf(buffer, sizeof(buffer), format, args...);
    return std::string(buffer);
}

// 简单字符串版本（不需要格式化参数）
template<typename ModuleType>
std::string formatLogMessage(const ModuleType& module,
                           const std::string& file,
                           int line,
                           const char* message) {
    return std::string(message);
}

// 日志记录函数
inline void writeLog(const std::string& module, LogLevel level,
                    const std::string& file, int line,
                    const std::string& message) {
    auto& manager = LogManager::getInstance();

    // 写入模块专用日志
    auto logger = manager.getLogger(module);
    if (logger) {
        logger->log(level, module, file, line, message);
    }

    // 同时写入汇总日志
    auto summary_logger = manager.getSummaryLogger();
    if (summary_logger) {
        summary_logger->log(level, module, file, line, message);
    }
}

} // namespace log_utils

// 为了向后兼容，保留 planner 命名空间的别名
namespace planner {
    using namespace log_utils;
}

// LOG_FILE 宏（与 LOG 相同，为了兼容性保留）
#define LOG_FILE(module, level, format, ...) LOG(module, level, format, ##__VA_ARGS__)

// 日志宏（仅输出到文件）
#define LOG(module, level, format, ...) \
    do { \
        const char* __module_str = #module; \
        std::string __message = log_utils::formatLogMessage(__module_str, __FILE__, __LINE__, format, ##__VA_ARGS__); \
        if (strcmp(#level, "INFO") == 0) { \
            log_utils::writeLog(__module_str, log_utils::LogLevel::INFO, __FILE__, __LINE__, __message); \
        } else if (strcmp(#level, "WARN") == 0) { \
            log_utils::writeLog(__module_str, log_utils::LogLevel::WARN, __FILE__, __LINE__, __message); \
        } else if (strcmp(#level, "ERROR") == 0) { \
            log_utils::writeLog(__module_str, log_utils::LogLevel::ERROR, __FILE__, __LINE__, __message); \
        } else if (strcmp(#level, "DEBUG") == 0) { \
            log_utils::writeLog(__module_str, log_utils::LogLevel::DEBUG, __FILE__, __LINE__, __message); \
        } \
    } while(0)

#define LOG_STREAM(module, level, stream) \
    do { \
        const char* __module_str = #module; \
        std::ostringstream __oss; \
        __oss << stream; \
        std::string __message = __oss.str(); \
        if (strcmp(#level, "INFO") == 0) { \
            log_utils::writeLog(__module_str, log_utils::LogLevel::INFO, __FILE__, __LINE__, __message); \
        } else if (strcmp(#level, "WARN") == 0) { \
            log_utils::writeLog(__module_str, log_utils::LogLevel::WARN, __FILE__, __LINE__, __message); \
        } else if (strcmp(#level, "ERROR") == 0) { \
            log_utils::writeLog(__module_str, log_utils::LogLevel::ERROR, __FILE__, __LINE__, __message); \
        } else if (strcmp(#level, "DEBUG") == 0) { \
            log_utils::writeLog(__module_str, log_utils::LogLevel::DEBUG, __FILE__, __LINE__, __message); \
        } \
    } while(0)

#endif // LOG_UTILS_LOG_UTILS_H