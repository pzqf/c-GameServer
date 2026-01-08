#pragma once

#include "network/SocketTypes.h"
#include "network/AsyncIO.h"

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>
#include <map>
#include <cstring>
#include <queue>
#include <map>

#include "ConfigManager.h"
#include "DatabaseManager.h"
#include "AccountDB.h"
#include "logging/Log.h"
#include "../messaging/message.h"
#include "../messaging/message_header.h"
#include "../handler/message_handler.h"
#include "network/INetworkEventListener.h"
#include "network/NetworkEventDispatcher.h"

// 前向声明
class MainLoop;
class INetworkEventListener;

class NetworkServer
{
private:
    socket_t serverSocket;
    std::unique_ptr<AsyncIOManager> asyncIoManager;
    std::mutex clientsMutex;
    std::atomic<bool> isRunning;
    ConfigManager* config;
    DatabaseManager* database;
    AccountDB* accountDb;
    
    // 服务器配置
    int port;
    int maxConnections;
    
    // 事件分发器 - 解耦网络层和业务层
    NetworkEventDispatcher eventDispatcher;
    
    // 消息缓冲区，用于处理分片数据
    std::map<socket_t, std::vector<uint8_t>> messageBuffers_;
    
    // 异步I/O事件处理
    void onAcceptEvent(const IOEvent& event);
    void onReadEvent(const IOEvent& event);
    void onWriteEvent(const IOEvent& event);
    void onErrorEvent(const IOEvent& event);
    
    // 网络事件回调
    void handleAsyncIOEvent(const IOEvent& event);
    
public:
    // 设置主循环引用，用于传递消息
    void setMainLoop(MainLoop* mainLoop) { mainLoop_ = mainLoop; }
 
    NetworkServer(ConfigManager* configManager, DatabaseManager* dbManager = nullptr);
    ~NetworkServer();
    
    // 禁用拷贝和移动
    NetworkServer(const NetworkServer&) = delete;
    NetworkServer& operator=(const NetworkServer&) = delete;
    NetworkServer(NetworkServer&&) = delete;
    NetworkServer& operator=(NetworkServer&&) = delete;
    
    // 异步I/O初始化
    bool initializeAsyncIO();
    void setupAsyncIOCallbacks();
    
    bool initialize();
    bool start();
    void stop();
    void shutdown();
    
    // 客户端管理
    void addClient(socket_t clientSocket);
    void removeClient(socket_t clientSocket);
    void broadcastMessage(const std::string& message);
    
    // 状态检查
    bool isServerRunning() const { return isRunning.load(); }
    int getActiveConnections() const;
    
    // 网络操作
    bool sendToClient(socket_t clientSocket, const std::string& message);
    bool sendResponseToClient(const std::string& clientId, ResponseType responseType, const std::string& message, const std::string& data);
    
    // 消息队列操作
    MessagePtr getNextMessage();
    
    // 事件监听器管理
    void addEventListener(std::shared_ptr<INetworkEventListener> listener);
    void removeEventListener(std::shared_ptr<INetworkEventListener> listener);
    
    // 异步I/O操作
    bool sendAsync(socket_t clientSocket, const std::string& message);
    bool startAsyncReceive(socket_t clientSocket);
    
    // 发送网络消息
    void sendNetworkMessage(socket_t clientSocket, const NetworkMessage& message);
    
    // 客户端消息处理
    void handleClient(socket_t clientSocket);
    
    // 消息处理方法
    std::string processLoginRequest(const std::string& request, NetworkServer* server);
    std::string receiveFromClient(socket_t clientSocket);
    
private:
    bool createSocket();
    bool bindSocket();
    bool listenForConnections();
    void acceptClients();
    
private:
    // 主循环引用
    MainLoop* mainLoop_ = nullptr;
    
    // 客户端套接字列表
    std::vector<socket_t> clientSockets;
    
    // 消息队列（向后兼容）
    std::queue<MessagePtr> messageQueue;
    
    // 客户端信息映射
    std::map<socket_t, std::string> clientInfo;
    
    // 网络事件处理回调
    friend void handleClient(socket_t clientSocket, NetworkServer* server);
};