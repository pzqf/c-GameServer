// GameServer.cpp: 定义应用程序的入口点。
//

#include "GameServer.h"
#include "logging/Log.h"
#include "../messaging/message.h"
#include "messaging/result.h"

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef _WINSOCKAPI_
        #define _WINSOCKAPI_
    #endif
    #include <windows.h>
#endif

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

using namespace std;



// 获取配置文件路径的辅助函数
std::string getConfigPath() {
    // 首先尝试获取可执行文件目录
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    
    // 提取目录部分
    std::string exePath(buffer);
    size_t lastSlash = exePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        std::string exeDir = exePath.substr(0, lastSlash);
        
        // 尝试项目根目录 (Debug的上级目录的上级目录)
        std::string configPath = exeDir + "\\..\\..\\config\\config.ini";
        
        // 检查这个路径是否存在
        HANDLE hFile = CreateFileA(configPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
            return configPath;
        }
        
        // 如果还是不存在，尝试Debug的上级目录
        size_t lastSlash2 = exeDir.find_last_of("\\/");
        if (lastSlash2 != std::string::npos) {
            std::string parentDir = exeDir.substr(0, lastSlash2);
            configPath = parentDir + "\\config\\config.ini";
            
            hFile = CreateFileA(configPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(hFile);
                return configPath;
            }
        }
    }
    
    // 如果都找不到，使用默认路径
    return "config\\config.ini";
}

// 全局服务器实例
NetworkServer* g_server = nullptr;

// 信号处理器
void signalHandler(int signal)
{
    cout << "\nReceived signal " << signal << ", shutting down gracefully..." << endl;
    if (g_server)
    {
        g_server->stop();
    }
}

// 应用程序类实现
GameServerApp::GameServerApp()
    : config(nullptr), server(nullptr), database(nullptr), mainLoop(nullptr), isRunning(false)
{
}

GameServerApp::~GameServerApp()
{
    shutdown();
}

bool GameServerApp::initialize()
{
    cout << "GameServer Starting..." << endl;
    cout << "=================================" << endl;
    
    try
    {
        // 加载配置
        cout << "Loading configuration from: config/config.ini" << endl;
        // 使用智能路径查找确保配置文件能被找到
        std::string configPath = getConfigPath();
        cout << "Full config path: " << configPath << endl;
        config = new ConfigManager(configPath);
        cout << "ConfigManager created " << endl;

        displayConfiguration();
        
        // 初始化日志系统
        cout << "=================================" << endl;
        cout << "Initializing logging system..." << endl;
        if (!Log::initialize(config))
        {
            cerr << "Failed to initialize logging system" << endl;
            return false;
        }
        cout << "Logging system initialized successfully" << endl;
        
        // 初始化数据库管理器显示系统信息
        //cout << "=================================" << endl;
        //cout << "Logging system information... " << endl;
        //logSystemInfo();
        //cout << "System information logged" << endl;
        
        // 初始化数据库管理器
        cout << "=================================" << endl;
        cout << "Initializing database manager... " << endl;
        if (!initializeDatabase(config))
        {
            cerr << "Failed to initialize database" << endl;
            return false;
        }
        cout << "Database manager initialized successfully" << endl;
        
        // 设置信号处理器
        cout << "=================================" << endl;
        cout << "Setting up signal handlers..." << endl;
        setupSignalHandlers();
        cout << "Signal handlers set up" << endl;
        
        // 创建网络服务器
        cout << "=================================" << endl;
        cout << "Creating network server..." << endl;
        server = new NetworkServer(config, &DatabaseManager::getInstance());
        cout << "Network server created" << endl;
        
        // 初始化服务器
        cout << "Initializing network server..." << endl;
        if (!server->initialize())
        {
            cerr << "Failed to initialize network server" << endl;
            return false;
        }
        cout << "Network server initialized successfully" << endl;
        
        // 创建主循环
        cout << "=================================" << endl;
        cout << "Creating main loop..." << endl;
        mainLoop = new MainLoop();
        cout << "Main loop created" << endl;
        
        // 将网络服务器引用传递给主循环
        mainLoop->setNetworkServer(server);
        cout << "Network server reference set in main loop" << endl;
        
        // 将主循环引用传递给网络服务器
        server->setMainLoop(mainLoop);
        cout << "Main loop reference set in network server" << endl;
        
        // 注册消息处理函数
        registerMessageHandlers();
        cout << "Message handlers registered" << endl;
        
        cout << "Application initialized successfully!" << endl;
        return true;
    }
    catch (const exception& e)
    {
        cerr << "Initialization failed: " << e.what() << " [CONSOLE ERROR]" << endl;
        return false;
    }
}

bool GameServerApp::start()
{
    if (!server)
    {
        LOG_ERROR("Server not initialized");
        return false;
    }
    
    LOG_INFO("Starting network server...");
    
    if (!server->start())
    {
        LOG_ERROR("Failed to start network server");
        return false;
    }
    
    isRunning = true;
    LOG_INFO("Server started successfully on port {}", config->getServerPort());
    LOG_INFO("Max connections: {}", config->getMaxConnections());
    LOG_INFO("Server is running. Press Ctrl+C to stop.");
    
    return true;
}

void GameServerApp::run()
{
    // 启动主循环
    if (mainLoop) {
        LOG_INFO("Starting main loop...");
        mainLoop->start();
        LOG_INFO("Main loop started");
    }
    
    while (isRunning && server && server->isServerRunning())
    {
        // 显示服务器状态
        LOG_INFO("\rActive connections: {} | Press Ctrl+C to stop", server->getActiveConnections());
        
        // 睡眠1秒
        this_thread::sleep_for(chrono::seconds(1));
    }
}

void GameServerApp::stop()
{
    LOG_INFO("Stopping application...");
    isRunning = false;
    
    if (server)
    {
        server->stop();
    }
}

void GameServerApp::shutdown()
{
    LOG_INFO("Shutting down...");
    
    stop();
    
    // 首先停止主循环
    if (mainLoop)
    {
        LOG_INFO("Stopping main loop...");
        mainLoop->stop();
        delete mainLoop;
        mainLoop = nullptr;
        LOG_INFO("Main loop stopped");
    }
    
    // 然后停止网络服务器
    if (server)
    {
        delete server;
        server = nullptr;
    }
    
    // 关闭数据库
    shutdownDatabase();
    
    if (config)
    {
        delete config;
        config = nullptr;
    }
    
    // 关闭日志系统
    Log::shutdown();
    
    LOG_INFO("Shutdown complete");
}

// 暂时注释掉loadConfiguration函数，避免编译错误
/*
void GameServerApp::loadConfiguration()
{
    string configFile = "config/config.ini";
    
    // 尝试从命令行参数获取配置文件路径
    for (int i = 1; i < __argc; i++)
    {
        if (string(__argv[i]) == "-c" || string(__argv[i]) == "--config")
        {
            if (i + 1 < __argc)
            {
                configFile = __argv[i + 1];
                break;
            }
        }
    }
    
    config = new ConfigManager(configFile);
    displayConfiguration();
}
*/

void GameServerApp::displayConfiguration()
{
    cout << "\nLoaded Configuration:" << endl;
    cout << "Server: " << config->getServerHost() << ":" << to_string(config->getServerPort()) << endl;
    cout << "Max Connections: " << to_string(config->getMaxConnections()) << endl;
    cout << "Database: " << config->getDatabaseHost() << ":" << to_string(config->getDatabasePort()) << "/" << config->getDatabaseName() << endl;
    cout << "Log Level: " << config->getLogLevel() << endl;
    cout << "Log File: " << config->getLogFilePath() << endl;
    cout << "Session Timeout: " << to_string(config->getSessionTimeout()) << " seconds" << endl;
    cout << "Thread Pool Size: " << to_string(config->getThreadPoolSize()) << endl;
}

void GameServerApp::setupSignalHandlers()
{
    // 在Windows上，信号处理比较有限
    // 这里主要为了Linux/Unix兼容性
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
}

void GameServerApp::logSystemInfo()
{
    cout << "\nSystem Information:" << endl;
    cout << "==================" << endl;
    
#ifdef _WIN32
    cout << "Platform: Windows" << endl;
#else
    cout << "Platform: Linux/Unix" << endl;
#endif
    
    cout << "Compiler: C++20" << endl;
    cout << "Build Time: " << string(__DATE__) << " " << string(__TIME__) << endl;
}

bool GameServerApp::initializeDatabase(ConfigManager* configManager)
{
    cout << "Initializing database connection pools..." << endl;
    
    try
    {
        // 初始化数据库管理器
        if (!DatabaseManager::getInstance().initialize(configManager))
        {
            cerr << "Failed to initialize database manager" << endl;
            return false;
        }
        
        cout << "Database connection pools initialized successfully" << endl;
        
        return true;
    }
    catch (const std::exception& e)
    {
        cerr << "Database initialization failed: " << string(e.what()) << " [CONSOLE ERROR]" << endl;
        return false;
    }
}

void GameServerApp::shutdownDatabase()
{
    LOG_INFO("Shutting down database connection pools...");
    DatabaseManager::getInstance().shutdown();
    LOG_INFO("Database connection pools shut down complete");
}

void GameServerApp::registerMessageHandlers()
{
    if (!mainLoop) {
        cerr << "Main loop is null, cannot register handlers" << endl;
        return;
    }
    
    AccountDB* accountDB = &AccountDB::getInstance();
    
    // 注册登录消息处理函数
    mainLoop->getHandler().registerHandler(MessageType::LOGIN, [this, accountDB](const Message& message) {
        if (auto* loginMsg = dynamic_cast<const LoginMessage*>(&message)) {
            LOG_INFO("Handling login message for user: {}", loginMsg->getUsername());
            
            try {
                bool success = accountDB->verifyPassword(loginMsg->getUsername(), loginMsg->getPassword());
                if (success) {
                    sendResponse(loginMsg->getClientId(), ResponseType::SUCCESS, "Login successful", "");
                } else {
                    sendResponse(loginMsg->getClientId(), ResponseType::SERVICE_ERROR, "Invalid credentials", "");
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to process login: {}", e.what());
                sendResponse(loginMsg->getClientId(), ResponseType::SERVICE_ERROR, "Failed to process login", "");
            }
        }
    });
    
    // 注册注册消息处理函数
    mainLoop->getHandler().registerHandler(MessageType::REGISTER, [this, accountDB](const Message& message) {
        if (auto* registerMsg = dynamic_cast<const RegisterMessage*>(&message)) {
            LOG_INFO("Handling register message for user: {}", registerMsg->getUsername());
            
            try {
                AccountInfo account;
                account.username = registerMsg->getUsername();
                account.password = registerMsg->getPassword();
                account.email = registerMsg->getEmail();
                account.status = "active";
                
                bool success = accountDB->createAccount(account);
                if (success) {
                    sendResponse(registerMsg->getClientId(), ResponseType::SUCCESS, "Registration successful", "");
                } else {
                    sendResponse(registerMsg->getClientId(), ResponseType::SERVICE_ERROR, "Registration failed", "");
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to process register: {}", e.what());
                sendResponse(registerMsg->getClientId(), ResponseType::SERVICE_ERROR, "Failed to process register", "");
            }
        }
    });
    
    LOG_INFO("Message handlers registered successfully");
}

void GameServerApp::sendResponse(const std::string& clientId, ResponseType responseType, const std::string& message, const std::string& data)
{
    LOG_INFO("Sending response to client {}: [{}] {}", clientId, static_cast<int>(responseType), message);
    
    if (server) {
        server->sendResponseToClient(clientId, responseType, message, data);
    } else {
        LOG_ERROR("Server is null, cannot send response");
    }
}

// 主函数
int main(int argc, char* argv[])
{
    GameServerApp app;
    
    // 设置全局服务器指针
    g_server = nullptr;
    
    int result = 0;
    
    try
    {
        // 初始化应用程序
        if (!app.initialize())
        {
            cerr << "Failed to initialize application" << endl;
            result = 1;
            return result;
        }
        
        // 启动服务器
        if (!app.start())
        {
            cerr << "Failed to start server" << endl;
            result = 1;
            return result;
        }
        
        // 运行主循环
        app.run();
        
        return 0;
    }
    catch (const exception& e)
    {
        cerr << "Fatal error: " << e.what() << endl;
        LOG_ERROR("Fatal error: {}", e.what());
        result = 1;
    }
    catch (...)
    {
        cerr << "Unknown fatal error occurred" << endl;
        LOG_ERROR("Unknown fatal error occurred");
        result = 1;
    }
    
    // 确保清理资源
    app.shutdown();
    
    // 确保日志被刷新到文件
    Log::flush();
    
    return result;
}
