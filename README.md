# Log Utils - 增强的日志系统

本包提供了一个增强的日志系统，专门负责将日志记录到文件中，不会干扰终端输出，并且在程序结束时自动导出日志。

## 功能特性

1. **文件专用日志**：仅输出到文件，不干扰终端输出
2. **汇总日志**：自动生成包含所有模块日志的汇总文件 `ALL_LOGS_SUMMARY.log`
3. **自动导出**：程序结束时自动导出所有日志
4. **路径集成**：与 `start_system.sh` 脚本的日志路径规范集成
5. **向后兼容**：完全兼容现有的 `LOG` 宏
6. **多级别支持**：支持 DEBUG、INFO、WARN、ERROR 四个日志级别
7. **线程安全**：支持多线程环境下的安全日志记录

## 使用方法

### 1. 基本用法（仅输出到文件）

```cpp
#include "log_utils/log_utils.h"

// 这些日志会同时写入两个地方：
// 1. 对应的模块日志文件（如 Planner.log、CONTROLLER.log）
// 2. 汇总日志文件（ALL_LOGS_SUMMARY.log）
LOG(Planner, INFO, "规划器已启动");
LOG(CONTROLLER, WARN, "控制器警告: 值 %f 超出范围", value);
LOG(SENSOR, ERROR, "传感器错误: 代码 %d", error_code);
LOG(DEBUG_MODULE, DEBUG, "调试信息: %s", debug_string);
```

### 2. 终端输出（需要自行处理）

```cpp
// 终端输出需要您自行使用 cout 或 ROS 日志
std::cout << "规划器已启动" << std::endl;
ROS_INFO("规划器已启动");
ROS_WARN("控制器警告: 值 %f 超出范围", value);
```

### 3. 高级用法

```cpp
// 直接使用 LogManager
auto logger = log_utils::LogManager::getInstance().getLogger("MY_MODULE");
logger->log(log_utils::LogLevel::INFO, "MY_MODULE", __FILE__, __LINE__, "自定义消息");

// 手动导出日志
log_utils::LogManager::getInstance().exportLogs();
```

## 环境变量

系统会自动从以下环境变量获取日志路径：

- `LOG_DIR`: 日志目录路径（由 start_system.sh 设置）
- `ROS_WORKSPACE`: ROS工作空间路径（备用路径）

如果这些环境变量都不存在，会使用默认路径 `/tmp/two_stage_int_logs`。

## 日志文件结构

日志文件会按模块名称分别保存，同时生成汇总文件：

```
$LOG_DIR/
├── ALL_LOGS_SUMMARY.log # 🆕 汇总所有模块的日志
├── Planner.log          # 规划器模块日志
├── CONTROLLER.log       # 控制器模块日志
├── SENSOR.log           # 传感器模块日志
├── intercepter_sim.log  # 终端输出重定向
└── debug.log           # 调试信息
```

### 汇总日志的优势

- **统一视图**：`ALL_LOGS_SUMMARY.log` 包含所有模块的日志，按时间顺序排列
- **便于调试**：可以在单个文件中查看整个系统的运行状态
- **模块化查看**：仍然保留各模块的独立日志文件，便于模块级调试

## 日志格式

文件中的日志格式为：
```
[2023-12-07 14:30:25.123] [INFO] [Planner] planner.cpp:42 - 规划器已启动
[2023-12-07 14:30:25.456] [WARN] [CONTROLLER] controller.cpp:15 - 控制器警告: 值 3.14 超出范围
```

## 自动导出

程序正常结束时，日志系统会自动：
1. 关闭所有打开的日志文件
2. 在终端显示导出位置
3. 列出所有生成的日志文件

## 与 start_system.sh 的集成

`start_system.sh` 脚本会：
1. 创建时间戳目录（如 `/logs/20231207_143025/`）
2. 设置 `LOG_DIR` 环境变量
3. 为每个tmux窗格传递正确的环境变量
4. 在系统关闭时显示日志位置

## 向后兼容性

现有代码无需修改即可使用新的日志系统。所有原有的 `LOG` 宏调用都会：
1. 写入对应的模块日志文件（如 `Planner.log`）
2. 同时写入汇总日志文件（`ALL_LOGS_SUMMARY.log`）
3. 不会在终端显示（终端输出需要您自行处理）

## 示例代码

参考 `log_example.h` 文件查看完整的使用示例。

## 性能考虑

- 日志写入使用互斥锁确保线程安全
- 每次写入后立即刷新缓冲区，确保数据不丢失
- 文件 I/O 采用追加模式，性能开销最小