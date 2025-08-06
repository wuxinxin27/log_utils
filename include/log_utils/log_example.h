#ifndef LOG_UTILS_LOG_EXAMPLE_H
#define LOG_UTILS_LOG_EXAMPLE_H

#include "log_utils/log_utils.h"

namespace log_utils {

/**
 * 使用示例：
 *
 * 1. 使用 LOG 宏（输出到模块日志和汇总日志，不影响终端）：
 *    LOG(Planner, INFO, "This is an info message: %d", 42);
 *    LOG(CONTROLLER, WARN, "Warning: value is %f", 3.14);
 *    // 会同时写入 Planner.log、CONTROLLER.log 和 ALL_LOGS_SUMMARY.log
 *
 * 2. 使用 LOG_FILE 宏（与 LOG 相同，为了兼容性保留）：
 *    LOG_FILE(Planner, INFO, "This only goes to file: %s", "test");
 *    LOG_FILE(CONTROLLER, ERROR, "Error occurred: %d", error_code);
 *
 * 3. 终端输出请自行处理：
 *    std::cout << "终端输出" << std::endl;
 *    ROS_INFO("ROS 日志输出");
 *
 * 4. 直接使用 LogManager：
 *    auto logger = LogManager::getInstance().getLogger("MY_MODULE");
 *    logger->log(LogLevel::INFO, "MY_MODULE", __FILE__, __LINE__, "Custom message");
 *
 * 5. 汇总日志包含所有模块的日志：
 *    ALL_LOGS_SUMMARY.log 文件会包含所有模块的日志，按时间顺序排列
 *
 * 6. 在程序结束时，日志会自动导出，您也可以手动导出：
 *    LogManager::getInstance().exportLogs();
 */

// 示例函数
inline void demonstrateLogging() {
    // 这些日志会同时写入 DEMO.log 和 ALL_LOGS_SUMMARY.log，不会在终端显示
    LOG(DEMO, INFO, "这是一个演示信息日志");
    LOG(DEMO, WARN, "这是一个警告日志，值: %d", 123);
    LOG(DEMO, ERROR, "这是一个错误日志");
    LOG(DEMO, DEBUG, "这是一个调试日志");

    // LOG_FILE 与 LOG 功能相同，也会写入汇总日志
    LOG_FILE(DEMO, INFO, "这也会写入模块日志和汇总日志");

    // 直接使用 LogManager 的高级用法
    auto custom_logger = LogManager::getInstance().getLogger("CUSTOM_MODULE", LogLevel::WARN);
    if (custom_logger) {
        custom_logger->log(LogLevel::WARN, "CUSTOM_MODULE", __FILE__, __LINE__,
                          "这是通过 LogManager 直接记录的日志");
    }

    // 终端输出需要自行处理
    std::cout << "终端输出示例（不会写入日志文件）" << std::endl;
    ROS_INFO("ROS 日志输出示例（不会写入日志文件）");

    // 查看日志文件位置
    std::cout << "日志文件保存在: " << LogManager::getInstance().getLogDirectory() << std::endl;
    std::cout << "汇总日志文件: " << LogManager::getInstance().getLogDirectory() << "/ALL_LOGS_SUMMARY.log" << std::endl;
}

} // namespace log_utils

#endif // LOG_UTILS_LOG_EXAMPLE_H