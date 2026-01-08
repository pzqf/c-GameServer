#include "Log.h"
#include "ConfigManager.h"

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef _WINSOCKAPI_
        #define _WINSOCKAPI_
    #endif
    #include <windows.h>
    #include <direct.h>
    #include <errno.h>
#else
    #include <unistd.h>
    #include <errno.h>
#endif

#include <cstdio>

#include <algorithm>
#include <iostream>
#include <thread>
#include <chrono>

bool Log::initialize(ConfigManager *config)
{
    if (initialized)
    {
        return true;
    }

    try
    {
        std::string logLevel = "info";
        std::string logPattern = "[%Y-%m-%d %H:%M:%S.%f] [%l] [%t] %v";
        std::string logFile = "logs/log.log";
        int maxFileSizeMB = 10; // 10MB
        int maxFiles = 5;
        bool enableConsole = true;
        bool enableFile = true;
        bool enableAsync = true; // 默认启用异步
        int asyncThreadCount = 4; // 异步日志线程数
        int asyncQueueSize = 8192; // 异步日志队列大小
        bool useDailyRotation = true;

        // 如果提供了ConfigManager，优先从ConfigManager读取配置
        if (config != nullptr)
        {
            logLevel = config->getLogLevel();
            enableConsole = config->isConsoleLogging();
            enableFile = config->isFileLogging();
            enableAsync = config->isAsyncLogging();
            asyncThreadCount = config->getAsyncThreadCount();
            asyncQueueSize = config->getAsyncQueueSize(); // 读取配置的队列大小
            logPattern = config->getLogPattern();
            logFile = config->getLogFilePath();
            maxFileSizeMB = config->getMaxFileSizeMB();
            maxFiles = config->getMaxFiles();
            useDailyRotation = config->isDailyRotation();
        }
       

        // 设置异步线程池（如果启用异步）
        if (enableAsync)
        {
            // 确保线程数至少为1，队列大小至少为1024
            int threadCount = asyncThreadCount > 0 ? asyncThreadCount : 1;
            int queueSize = asyncQueueSize > 0 ? asyncQueueSize : 8192;
            spdlog::init_thread_pool(queueSize, threadCount); // 使用配置的队列大小和工作线程数
        }

        // 设置全局日志级别
        spdlog::level::level_enum globalLevel = getSpdLogLevel(logLevel);
        spdlog::set_level(globalLevel);
        spdlog::set_pattern(logPattern);

        std::vector<spdlog::sink_ptr> sinks;

        // 控制台输出
        if (enableConsole)
        {
            auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            consoleSink->set_pattern(logPattern);
            sinks.push_back(consoleSink);
        }

        // 文件输出
        if (enableFile)
        {
            std::string logDir = getLogDirectory();
            std::cout << "Creating log directory: " << logDir << std::endl;
            createLogDirectory(logDir);

            std::cout << "Log file path: " << logFile << std::endl;

            if (useDailyRotation)
            {
                // 每日轮转日志
                auto dailySink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                    logFile, 0, 0); // 每天0点轮转
                sinks.push_back(dailySink);
            }
            else
            {
                // 固定大小轮转日志
                int fileSizeBytes = maxFileSizeMB * 1024 * 1024; // 转换为字节
                auto rotatingSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    logFile, fileSizeBytes, maxFiles);
                sinks.push_back(rotatingSink);
            }
        }

        if (sinks.empty())
        {
            // 如果没有配置任何sink，使用默认控制台sink
            auto defaultSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            sinks.push_back(defaultSink);
        }

        // 创建日志器
        if (enableAsync)
        {
            logger = std::make_shared<spdlog::async_logger>("main",
                                                            sinks.begin(), sinks.end(), spdlog::thread_pool(),
                                                            spdlog::async_overflow_policy::block);

            logger->set_pattern(logPattern);
            spdlog::register_logger(logger);
        }
        else
        {
            logger = std::make_shared<spdlog::logger>("main", sinks.begin(), sinks.end());
        }

        spdlog::flush_every(std::chrono::seconds(3));
        logger->set_level(getSpdLogLevel(logLevel));
        logger->set_pattern(logPattern);

        if (logger)
        {
            initialized = true;
            
            // 输出初始化信息
            logger->info("=== Log System Initialized ===");
            logger->info("Level: {}, Console: {}, File: {}, Async: {}", 
                        logLevel, enableConsole, enableFile, enableAsync);
            logger->info("Pattern: {}", logPattern);
            logger->info("Log file: {}", logFile);
            if (enableAsync)
            {
                logger->info("Async threads: {}, Queue size: {}", asyncThreadCount, asyncQueueSize);
            }
            logger->info("============================");
            
            return true;
        }

        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to initialize Log system: " << e.what() << std::endl;
        return false;
    }
}



void Log::shutdown()
{
    if (!initialized)
    {
        return;
    }

    try
    {
        if (logger)
        {
            logger->info("Log system shutting down...");
            logger->flush();
        }

        // 清理
        if (logger)
        {
            spdlog::drop(logger->name());
            logger.reset();
        }

        initialized = false;

        // 关闭spdlog
        spdlog::shutdown();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error during Log shutdown: " << e.what() << std::endl;
    }
}

void Log::flush()
{
    if (initialized && logger)
    {
        logger->flush();
    }
}

void Log::trace(const std::string &message)
{
    if (initialized && logger)
    {
        logger->trace(message);
    }
}

void Log::debug(const std::string &message)
{
    if (initialized && logger)
    {
        logger->debug(message);
    }
}

void Log::info(const std::string &message)
{
    if (initialized && logger)
    {
        logger->info(message);
    }
}

void Log::warn(const std::string &message)
{
    if (initialized && logger)
    {
        logger->warn(message);
    }
}

void Log::error(const std::string &message)
{
    if (initialized && logger)
    {
        logger->error(message);
    }
}

void Log::critical(const std::string &message)
{
    if (initialized && logger)
    {
        logger->critical(message);
    }
}

spdlog::level::level_enum Log::getSpdLogLevel(const std::string &level)
{
    std::string lowerLevel = level;
    std::transform(lowerLevel.begin(), lowerLevel.end(), lowerLevel.begin(), ::tolower);

    if (lowerLevel == "trace" || lowerLevel == "verbose")
        return spdlog::level::trace;
    else if (lowerLevel == "debug")
        return spdlog::level::debug;
    else if (lowerLevel == "info")
        return spdlog::level::info;
    else if (lowerLevel == "warning" || lowerLevel == "warn")
        return spdlog::level::warn;
    else if (lowerLevel == "error" || lowerLevel == "err")
        return spdlog::level::err;
    else if (lowerLevel == "critical" || lowerLevel == "fatal")
        return spdlog::level::critical;
    else
        return spdlog::level::info;
}

std::string Log::getLogDirectory()
{
    // 获取可执行文件目录
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);

    std::string exePath(buffer);
    size_t lastSlash = exePath.find_last_of("\\/");
    if (lastSlash != std::string::npos)
    {
        std::string exeDir = exePath.substr(0, lastSlash);
        std::string logDir = exeDir + "\\logs";

        // 输出调试信息
        std::cout << "Executable path: " << exePath << std::endl;
        std::cout << "Log directory: " << logDir << std::endl;

        return logDir;
    }

    // 如果获取失败，返回绝对路径
    return "D:\\work\\AccountSvr\\vs_project\\Debug\\logs";
}

void Log::createLogDirectory(const std::string &directory)
{
#ifdef _WIN32
    // 创建日志目录
    if (CreateDirectoryA(directory.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // 目录创建成功或已存在
    }
    else
    {
        // 打印错误信息
        DWORD error = GetLastError();
        std::cerr << "Failed to create log directory: " << directory << ", Error: " << error << std::endl;
    }
#else
    if (mkdir(directory.c_str(), 0755) == -1 && errno != EEXIST)
    {
        std::cerr << "Failed to create log directory: " << directory << ", Error: " << strerror(errno) << std::endl;
    }
#endif
}
