// AccountSvr.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <signal.h>
#include "INIReader.h"
#include "ConfigManager.h"
#include "Log.h"
#include "NetworkServer.h"
#include "DatabaseManager.h"
#include "messaging/result.h"

#include "main/MainLoop.h"

// 全局服务器实例指针
extern NetworkServer* g_server;

// 信号处理器
void signalHandler(int signal);

// 应用程序类
class GameServerApp
{
private:
    ConfigManager* config;

    NetworkServer* server;
    DatabaseManager* database;
    MainLoop* mainLoop;
    bool isRunning;

public:
    GameServerApp();
    ~GameServerApp();

    bool initialize();
    bool start();
    void stop();
    void run();
    void shutdown();

    // 配置管理
    void loadConfiguration();
    void displayConfiguration();

    // 数据库管理
    bool initializeDatabase(ConfigManager* configManager);
    void shutdownDatabase();
    
    // 消息处理
    void registerMessageHandlers();
    void sendResponse(const std::string& clientId, ResponseType responseType, const std::string& message, const std::string& data);

private:
    void setupSignalHandlers();
    void logSystemInfo();
};
