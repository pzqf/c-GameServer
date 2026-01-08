#pragma once

#include <string>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/async.h>
#include <spdlog/fmt/fmt.h>

// 前向声明
class ConfigManager;

// 便捷宏定义
#define LOG_TRACE(...) Log::trace(__VA_ARGS__)
#define LOG_DEBUG(...) Log::debug(__VA_ARGS__)
#define LOG_INFO(...)  Log::info(__VA_ARGS__)
#define LOG_WARN(...)  Log::warn(__VA_ARGS__)
#define LOG_ERROR(...) Log::error(__VA_ARGS__)
#define LOG_CRITICAL(...) Log::critical(__VA_ARGS__)

class Log
{
private:
    inline static std::shared_ptr<spdlog::logger> logger;
    inline static bool initialized = false;


public:
    // 初始化和清理
    // 支持直接参数初始化或从ConfigManager配置初始化
    static bool initialize(ConfigManager* config = nullptr);

    // 从ConfigManager初始化的便捷方法（向后兼容）
    static bool initializeFromConfig(ConfigManager& config);
    
    static void shutdown();
    static void flush();

    // 日志级别方法
    static void trace(const std::string& message);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    static void critical(const std::string& message);

    // 模板方法支持格式化日志
    template<typename... Args>
    static void trace(fmt::format_string<Args...> fmt, Args&&... args) {
        if (initialized && logger) {
            logger->trace(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    static void debug(fmt::format_string<Args...> fmt, Args&&... args) {
        if (initialized && logger) {
            logger->debug(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    static void info(fmt::format_string<Args...> fmt, Args&&... args) {
        if (initialized && logger) {
            logger->info(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    static void warn(fmt::format_string<Args...> fmt, Args&&... args) {
        if (initialized && logger) {
            logger->warn(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    static void error(fmt::format_string<Args...> fmt, Args&&... args) {
        if (initialized && logger) {
            logger->error(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    static void critical(fmt::format_string<Args...> fmt, Args&&... args) {
        if (initialized && logger) {
            logger->critical(fmt, std::forward<Args>(args)...);
        }
    }

    // 配置方法
    static bool isInitialized() { return initialized; }
    static std::string getLogDirectory();

private:
    static spdlog::level::level_enum getSpdLogLevel(const std::string& level);
    static void createLogDirectory(const std::string& directory);
};

// 便捷函数，使用命名空间避免命名冲突
namespace Logging {
    inline void trace(const std::string& message) { Log::trace(message); }
    inline void debug(const std::string& message) { Log::debug(message); }
    inline void info(const std::string& message) { Log::info(message); }
    inline void warn(const std::string& message) { Log::warn(message); }
    inline void error(const std::string& message) { Log::error(message); }
    inline void critical(const std::string& message) { Log::critical(message); }

    template<typename... Args>
    inline void trace(const std::string& fmt, const Args&... args) { Log::trace(fmt, args...); }

    template<typename... Args>
    inline void debug(const std::string& fmt, const Args&... args) { Log::debug(fmt, args...); }

    template<typename... Args>
    inline void info(const std::string& fmt, const Args&... args) { Log::info(fmt, args...); }

    template<typename... Args>
    inline void warn(const std::string& fmt, const Args&... args) { Log::warn(fmt, args...); }

    template<typename... Args>
    inline void error(const std::string& fmt, const Args&... args) { Log::error(fmt, args...); }

    template<typename... Args>
    inline void critical(const std::string& fmt, const Args&... args) { Log::critical(fmt, args...); }
}