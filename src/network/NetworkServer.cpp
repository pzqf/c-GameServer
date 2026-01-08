#include "NetworkServer.h"
#include "SocketTypes.h"
#include "AsyncIO.h"

#include "logging/Log.h"
#include "../messaging/message_header.h"
#include "main/MainLoop.h"
#include <cstdlib>
#include <chrono>

// 平台特定网络头文件
#ifdef _WIN32
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <cstring>
#endif

NetworkServer::NetworkServer(ConfigManager* configManager, DatabaseManager* dbManager)
    : serverSocket(INVALID_SOCKET_VALUE), asyncIoManager(std::make_unique<AsyncIOManager>()), 
      isRunning(false), config(configManager), database(dbManager), accountDb(nullptr)
{
    // 从配置管理器获取服务器设置
    port = config->getServerPort();
    maxConnections = config->getMaxConnections();
}

NetworkServer::~NetworkServer()
{
    shutdown();
}

bool NetworkServer::initialize()
{
    LOG_INFO("Initializing network server...");
    
    // 初始化异步I/O管理器
    if (!initializeAsyncIO())
    {
        return false;
    }
    
    // 创建服务器套接字
    if (!createSocket())
    {
        return false;
    }
    
    // 绑定套接字
    if (!bindSocket())
    {
        return false;
    }
    
    // 开始监听
    if (!listenForConnections())
    {
        return false;
    }
    
    // 注册服务器套接字到异步I/O管理器
    if (!asyncIoManager->addSocket(serverSocket, IOEventType::READ | IOEventType::IOERROR))
    {
        LOG_ERROR("Failed to register server socket with async I/O manager: {}", GET_LAST_ERROR());
        return false;
    }
    
    // 初始化数据库
    accountDb = &AccountDB::getInstance();
    
    LOG_INFO("Server initialized successfully on port {}", port);
    
    return true;
}

bool NetworkServer::initializeAsyncIO()
{
    if (!asyncIoManager) {
        return false;
    }
    
    if (!asyncIoManager->initialize()) {
        LOG_ERROR("Failed to initialize AsyncIOManager");
        return false;
    }
    
    // 设置异步I/O回调
    setupAsyncIOCallbacks();
    
    LOG_INFO("AsyncIOManager initialized successfully");
    return true;
}

void NetworkServer::setupAsyncIOCallbacks()
{
    if (!asyncIoManager) {
        return;
    }
    
    // 设置统一的网络事件回调
    asyncIoManager->setAcceptCallback([this](const IOEvent& event) {
        handleAsyncIOEvent(event);
    });
    
    asyncIoManager->setReadCallback([this](const IOEvent& event) {
        handleAsyncIOEvent(event);
    });
    
    asyncIoManager->setWriteCallback([this](const IOEvent& event) {
        handleAsyncIOEvent(event);
    });
    
    asyncIoManager->setErrorCallback([this](const IOEvent& event) {
        handleAsyncIOEvent(event);
    });
}

bool NetworkServer::createSocket()
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET_VALUE)
    {
        LOG_ERROR("Failed to create socket: {}", GET_LAST_ERROR());
        return false;
    }
    
    // 设置套接字选项
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR_VAL)
    {
        LOG_ERROR("Failed to set socket options: {}", GET_LAST_ERROR());
        CLOSE_SOCKET(serverSocket);
        serverSocket = INVALID_SOCKET_VALUE;
        return false;
    }
    
#ifndef _WIN32
    // 设置为非阻塞模式 (仅在Unix系统)
    int flags = fcntl(serverSocket, F_GETFL, 0);
    if (flags < 0 || fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        LOG_ERROR("Failed to set non-blocking mode: {}", strerror(errno));
        CLOSE_SOCKET(serverSocket);
        serverSocket = INVALID_SOCKET_VALUE;
        return false;
    }
#endif
    
    return true;
}

bool NetworkServer::bindSocket()
{
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR_VAL)
    {
        LOG_ERROR("Bind failed: {}", GET_LAST_ERROR());
        CLOSE_SOCKET(serverSocket);
        serverSocket = INVALID_SOCKET_VALUE;
        return false;
    }
    
    return true;
}

bool NetworkServer::listenForConnections()
{
    if (listen(serverSocket, maxConnections) == SOCKET_ERROR_VAL)
    {
        LOG_ERROR("Listen failed: {}", GET_LAST_ERROR());
        CLOSE_SOCKET(serverSocket);
        serverSocket = INVALID_SOCKET_VALUE;
        return false;
    }
    
    return true;
}

bool NetworkServer::start()
{
    LOG_INFO("Starting server...");
    isRunning = true;
    
    // 启动异步I/O事件循环
    if (!asyncIoManager->startEventLoop())
    {
        LOG_ERROR("Failed to start async I/O event loop: {}", GET_LAST_ERROR());
        isRunning = false;
        return false;
    }
    
    LOG_INFO("Server started successfully");
    return true;
}

// 统一的网络事件处理器
void NetworkServer::handleAsyncIOEvent(const IOEvent& event) {
    switch (event.eventType) {
        case IOEventType::ACCEPT:
            onAcceptEvent(event);
            break;
        case IOEventType::READ:
            onReadEvent(event);
            break;
        case IOEventType::WRITE:
            onWriteEvent(event);
            break;
        case IOEventType::IOERROR:
        default:
            onErrorEvent(event);
            break;
    }
}

void NetworkServer::onAcceptEvent(const IOEvent& event)
{
    if (event.socket != serverSocket) {
        LOG_WARN("Received accept event for non-server socket");
        return;
    }
    
    LOG_DEBUG("Processing accept event for server socket");
    
    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    
    socket_t clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET_VALUE)
    {
        if (isRunning.load())
        {
        #ifdef _WIN32
            int errorCode = WSAGetLastError();
            if (errorCode != WSAEWOULDBLOCK)
            {
                LOG_ERROR("Accept failed: {}", errorCode);
            }
        #else
            int errorCode = errno;
            if (errorCode != EAGAIN && errorCode != EWOULDBLOCK)
            {
                LOG_ERROR("Accept failed: {}", strerror(errorCode));
            }
        #endif
        }
        return;
    }
    
#ifndef _WIN32
    // 设置客户端套接字为非阻塞模式 (仅在Unix系统)
    int flags = fcntl(clientSocket, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
    }
#endif
    
    // 获取客户端IP地址
    #ifdef _WIN32
        char clientIP[46]; // Windows下，IPv6地址的最大长度是46字符
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, sizeof(clientIP));
    #else
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, sizeof(clientIP));
    #endif
    LOG_INFO("New client connected from {}:{}", clientIP, ntohs(clientAddr.sin_port));
    
    // 添加客户端到列表
    addClient(clientSocket);
    
    // 注册客户端套接字到异步I/O管理器
    if (!asyncIoManager->addClient(clientSocket))
    {
        LOG_ERROR("Failed to register client socket with async I/O manager: {}", GET_LAST_ERROR());
        removeClient(clientSocket);
        CLOSE_SOCKET(clientSocket);
        return;
    }
    
    // 发送欢迎消息
    sendToClient(clientSocket, "Welcome to Account Server! Please login.");
    
    // 分发客户端连接事件
    eventDispatcher.notifyClientConnected(clientSocket);
}

void NetworkServer::onReadEvent(const IOEvent& event)
{
    if (event.data.empty()) {
        LOG_DEBUG("Received empty read event for socket {}", event.socket);
        return;
    }
    
    LOG_DEBUG("Processing read event for socket {} with {} bytes", event.socket, event.data.size());
    
    // 将数据转换为字节向量并分发给监听器
    std::vector<uint8_t> data(event.data.begin(), event.data.end());
    eventDispatcher.notifyDataReceived(event.socket, data);
    
    // 将数据处理逻辑也在这里处理，以保持一致性
    // 注意：这里的消息处理逻辑与processNetworkMessage相同，保持一致性
    try {
        // 获取或创建客户端的消息缓冲区
        auto& buffer = messageBuffers_[event.socket];
        buffer.insert(buffer.end(), data.begin(), data.end());
        
        // 尝试解析完整消息
        while (buffer.size() >= sizeof(MessageHeader)) {
            // 检查是否包含完整消息
            if (!MessageParser::isCompleteMessage(buffer)) {
                break; // 数据不完整，等待更多数据
            }
            
            // 解析消息
            auto message = MessageParser::parseMessage(buffer);
            if (!message) {
                LOG_WARN("Failed to parse message from client {}", event.socket);
                // 清除缓冲区以避免无限循环
                buffer.clear();
                break;
            }
            
            LOG_DEBUG("Processing message ID {} from client {}", 
                     message->getHeader().messageId, event.socket);
            
            // 使用主循环处理消息 - 如果主循环存在，将消息转换为Message后添加到队列
            if (mainLoop_) {
                auto messagePtr = convertNetworkMessageToMessage(*message, event.socket);
                if (messagePtr) {
                    mainLoop_->addMessage(std::move(messagePtr));
                }
            }
            
            // 从缓冲区中移除已处理的消息
            size_t messageSize = message->getTotalSize();
            if (messageSize <= buffer.size()) {
                buffer.erase(buffer.begin(), buffer.begin() + messageSize);
            } else {
                buffer.clear(); // 防止数据不一致
                break;
            }
        }
        
        // 如果缓冲区太大，清理它（防止内存攻击）
        if (buffer.size() > 64 * 1024) { // 64KB
            LOG_WARN("Message buffer too large for client {}, clearing", event.socket);
            buffer.clear();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception processing network message from client {}: {}", event.socket, e.what());
        // 发送错误响应
        NetworkMessage errorResponse = MessageUtils::createErrorResponse(0, "Internal server error");
        sendNetworkMessage(event.socket, errorResponse);
    }
}

void NetworkServer::onWriteEvent(const IOEvent& event)
{
    LOG_DEBUG("Processing write event for socket {}", event.socket);
    
    // 分发数据发送事件
    eventDispatcher.notifyDataSent(event.socket, 0); // 这里可以传递实际发送的字节数
}

void NetworkServer::onErrorEvent(const IOEvent& event)
{
    LOG_DEBUG("Processing error event for socket {}", event.socket);
    
    // 移除客户端套接字并关闭连接
    removeClient(event.socket);
    CLOSE_SOCKET(event.socket);
    
    // 分发客户端断开连接事件
    eventDispatcher.notifyClientDisconnected(event.socket);
}

void NetworkServer::handleClient(socket_t clientSocket)
{
    LOG_INFO("Client {} thread started", clientSocket);
    
    // 发送欢迎消息
    sendToClient(clientSocket, "Welcome to Account Server! Please login.");
    
    char buffer[1024];
    while (isServerRunning())
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived <= 0)
        {
#ifndef _WIN32
            if (bytesReceived < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // 非阻塞模式下的正常情况，短暂休眠
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
#endif
            LOG_INFO("Client {} disconnected", clientSocket);
            break;
        }
        
        std::string request(buffer, bytesReceived);
        LOG_DEBUG("Received from client {}: '{}'", clientSocket, request);
        LOG_DEBUG("Message length: {}", bytesReceived);
        
        // 解析消息类型并创建相应的消息对象
        MessagePtr message = nullptr;
        
        // 简单的消息解析 - 格式: LOGIN:username:password
        if (request.substr(0, 6) == "LOGIN:")
        {
            // 解析用户名和密码
            size_t firstColon = request.find(':', 6);
            size_t secondColon = request.find(':', firstColon + 1);
            
            if (firstColon != std::string::npos && secondColon != std::string::npos)
            {
                std::string username = request.substr(firstColon + 1, secondColon - firstColon - 1);
                std::string password = request.substr(secondColon + 1);
                
                // 移除可能的换行符和回车符
                username.erase(std::remove(username.begin(), username.end(), '\r'), username.end());
                username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
                password.erase(std::remove(password.begin(), password.end(), '\r'), password.end());
                password.erase(std::remove(password.begin(), password.end(), '\n'), password.end());
                
                // 创建登录消息，包含客户端ID
                std::string clientId = std::to_string(clientSocket);
                message = std::make_unique<LoginMessage>(username, password, clientId);
            }
        }
        // 注册消息格式: REGISTER:username:password:email
        else if (request.substr(0, 9) == "REGISTER:")
        {
            size_t firstColon = request.find(':', 9);
            size_t secondColon = request.find(':', firstColon + 1);
            size_t thirdColon = request.find(':', secondColon + 1);
            
            if (firstColon != std::string::npos && secondColon != std::string::npos && thirdColon != std::string::npos)
            {
                std::string username = request.substr(firstColon + 1, secondColon - firstColon - 1);
                std::string password = request.substr(secondColon + 1, thirdColon - secondColon - 1);
                std::string email = request.substr(thirdColon + 1);
                
                // 移除可能的换行符和回车符
                username.erase(std::remove(username.begin(), username.end(), '\r'), username.end());
                username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
                password.erase(std::remove(password.begin(), password.end(), '\r'), password.end());
                password.erase(std::remove(password.begin(), password.end(), '\n'), password.end());
                email.erase(std::remove(email.begin(), email.end(), '\r'), email.end());
                email.erase(std::remove(email.begin(), email.end(), '\n'), email.end());
                
                // 创建注册消息，包含客户端ID
                std::string clientId = std::to_string(clientSocket);
                message = std::make_unique<RegisterMessage>(username, password, email, clientId);
            }
        }
        
        // 如果成功创建了消息，将其添加到队列中
        if (message)
        {
            try {
                // 如果主循环存在，直接将消息添加到主循环的消息队列
                if (mainLoop_) {
                    mainLoop_->addMessage(std::move(message));
                }
                else {
                    // 如果没有主循环，则使用网络服务器自己的消息队列（向后兼容）
                    messageQueue.push(std::move(message));
                }
                
                // 发送确认消息给客户端
                sendToClient(clientSocket, "Message received and queued for processing.");
            }
            catch (const std::exception& e) {
                LOG_ERROR("Failed to queue message: {}", e.what());
                sendToClient(clientSocket, "ERROR: Failed to queue message.");
            }
        }
        else
        {
            // 无法识别的消息格式
            sendToClient(clientSocket, "ERROR: Unrecognized message format.");
        }
        
        // 如果是登录请求，并且解析成功，我们仍然需要等待主线程处理
        // 所以这里不应该断开连接
    }
    
    removeClient(clientSocket);
    CLOSE_SOCKET(clientSocket);
    LOG_INFO("Client {} thread ended", clientSocket);
}

std::string NetworkServer::processLoginRequest(const std::string& request, NetworkServer* server)
{
    // 简单的登录协议格式: LOGIN:username:password
    if (request.substr(0, 6) == "LOGIN:")
    {
        // 解析用户名和密码
        size_t firstColon = request.find(':', 6);
        size_t secondColon = request.find(':', firstColon + 1);
        
        if (firstColon != std::string::npos && secondColon != std::string::npos)
        {
            // firstColon是第一个冒号的位置，firstColon + 1是用户名的开始
            std::string username = request.substr(firstColon + 1, secondColon - firstColon - 1);
            // secondColon + 1是密码的开始，到字符串末尾
            std::string password = request.substr(secondColon + 1);
            
            // 移除可能的换行符和回车符
            username.erase(std::remove(username.begin(), username.end(), '\r'), username.end());
            username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
            password.erase(std::remove(password.begin(), password.end(), '\r'), password.end());
            password.erase(std::remove(password.begin(), password.end(), '\n'), password.end());
            
            // 添加调试日志
            LOG_DEBUG("Parsed username: '{}'", username);
            LOG_DEBUG("Password length: {}", password.length());
            
            // 连接到数据库验证账号
            AccountDB& accountDb = AccountDB::getInstance();
            AccountInfo account;
            
            LOG_DEBUG("Looking up account for username: '{}'", username);
            
            if (accountDb.getAccountByUsername(username, account))
            {
                // 验证密码
                if (account.password == password)
                {
                    // 登录成功
                    LOG_INFO("User '{}' logged in successfully", username);
                    return "SUCCESS:Login successful! Welcome " + username;
                }
                else
                {
                    // 密码错误
                    LOG_WARN("User '{}' provided incorrect password", username);
                    return "ERROR:Invalid username or password";
                }
            }
            else
            {
                // 账号不存在
                LOG_WARN("Login attempt for non-existent account '{}'", username);
                return "ERROR:Invalid username or password";
            }
        }
    }
    
    return "ERROR:Invalid login format. Use LOGIN:username:password";
}

bool NetworkServer::sendToClient(socket_t clientSocket, const std::string& message)
{
    std::string fullMessage = message + "\n";
    int bytesSent = send(static_cast<SOCKET>(clientSocket), fullMessage.c_str(), static_cast<int>(fullMessage.length()), 0);
    
    if (bytesSent == SOCKET_ERROR_VAL)
    {
        LOG_ERROR("Send to client {} failed: {}", static_cast<int>(clientSocket), GET_LAST_ERROR());
        return false;
    }
    
    return true;
}

bool NetworkServer::sendResponseToClient(const std::string& clientId, ResponseType responseType, const std::string& message, const std::string& data)
{
    try {
        int clientSocket = std::stoi(clientId);
        std::string response = "RESPONSE:" + std::to_string(static_cast<int>(responseType)) + ":" + message;
        if (!data.empty()) {
            response += ":" + data;
        }
        return sendToClient(clientSocket, response);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to send response to client {}: {}", clientId, e.what());
        return false;
    }
}

std::string NetworkServer::receiveFromClient(socket_t clientSocket)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0)
    {
        return "";
    }
    
    return std::string(buffer, bytesReceived);
}

void NetworkServer::addClient(socket_t clientSocket)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    clientSockets.push_back(clientSocket);
    clientInfo[clientSocket] = "Connected";
}

void NetworkServer::removeClient(socket_t clientSocket)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    auto it = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);
    if (it != clientSockets.end())
    {
        clientSockets.erase(it);
    }
    
    clientInfo.erase(clientSocket);
    
    // 清理消息缓冲区
    messageBuffers_.erase(clientSocket);
}

void NetworkServer::broadcastMessage(const std::string& message)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    for (socket_t clientSocket : clientSockets)
    {
        sendToClient(clientSocket, message);
    }
}

int NetworkServer::getActiveConnections() const
{
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(clientsMutex));
    return static_cast<int>(clientSockets.size());
}

MessagePtr NetworkServer::getNextMessage()
{
    if (!messageQueue.empty()) {
        MessagePtr msg = std::move(const_cast<MessagePtr&>(messageQueue.front()));
        messageQueue.pop();
        return msg;
    }
    return nullptr;
}

void NetworkServer::stop()
{
    LOG_INFO("Stopping server...");
    isRunning = false;
}

// 异步I/O操作实现
bool NetworkServer::sendAsync(socket_t clientSocket, const std::string& message)
{
    LOG_DEBUG("Starting async send to socket {}", clientSocket);
    
    if (!asyncIoManager) {
        LOG_ERROR("AsyncIOManager is not initialized");
        return false;
    }
    
    // 发送消息并等待回调
    bool success = asyncIoManager->asyncWrite(clientSocket, message, [this, clientSocket](const IOEvent& event) {
        if (event.eventType == IOEventType::IOERROR) {
            LOG_ERROR("Async write failed for socket {}", clientSocket);
            removeClient(clientSocket);
            CLOSE_SOCKET(clientSocket);
        } else {
            LOG_DEBUG("Async write completed for socket {}", clientSocket);
        }
    });
    
    if (!success) {
        LOG_ERROR("Failed to start async write for socket {}: {}", clientSocket, GET_LAST_ERROR());
        return false;
    }
    
    return true;
}

bool NetworkServer::startAsyncReceive(socket_t clientSocket)
{
    LOG_DEBUG("Starting async receive for socket {}", clientSocket);
    
    if (!asyncIoManager) {
        LOG_ERROR("AsyncIOManager is not initialized");
        return false;
    }
    
    // 启动异步接收
    bool success = asyncIoManager->asyncRead(clientSocket, "", [this, clientSocket](const IOEvent& event) {
        if (event.eventType == IOEventType::IOERROR) {
            LOG_ERROR("Async read failed for socket {}", clientSocket);
            removeClient(clientSocket);
            CLOSE_SOCKET(clientSocket);
        } else if (!event.data.empty()) {
            LOG_DEBUG("Async read completed with {} bytes for socket {}", event.data.size(), clientSocket);
            
            // 创建临时IOEvent来处理读取的数据
            IOEvent readEvent;
            readEvent.socket = clientSocket;
            readEvent.eventType = IOEventType::READ;
            readEvent.data = event.data;
            
            // 处理读取的数据
            onReadEvent(readEvent);
            
            // 继续接收下一次数据
            startAsyncReceive(clientSocket);
        }
    });
    
    if (!success) {
        LOG_ERROR("Failed to start async read for socket {}: {}", clientSocket, GET_LAST_ERROR());
        return false;
    }
    
    return true;
}

void NetworkServer::shutdown()
{
    stop();
    
    // 关闭异步I/O管理器
    if (asyncIoManager) {
        asyncIoManager->shutdown();
    }
    
    // 关闭所有客户端连接
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (socket_t clientSocket : clientSockets)
    {
        CLOSE_SOCKET(clientSocket);
    }
    clientSockets.clear();
    clientInfo.clear();
    
    // 关闭服务器套接字
    if (serverSocket != INVALID_SOCKET_VALUE)
    {
        CLOSE_SOCKET(serverSocket);
        serverSocket = INVALID_SOCKET_VALUE;
    }
    
    LOG_INFO("Server shutdown complete");
}

// 发送网络消息
void NetworkServer::sendNetworkMessage(socket_t clientSocket, const NetworkMessage& message) {
    try {
        auto data = message.serialize();
        
        // 使用现有的sendAsync方法发送数据
        std::string dataStr(data.begin(), data.end());
        sendAsync(clientSocket, dataStr);
        
        LOG_DEBUG("Sent message ID {} to client {}", message.getHeader().messageId, clientSocket);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to send network message to client {}: {}", clientSocket, e.what());
    }
}

// 事件监听器管理实现
void NetworkServer::addEventListener(std::shared_ptr<INetworkEventListener> listener) {
    if (listener) {
        eventDispatcher.addListener(listener);
        LOG_INFO("Added event listener: {}", listener->getListenerName());
    }
}

void NetworkServer::removeEventListener(std::shared_ptr<INetworkEventListener> listener) {
    if (listener) {
        eventDispatcher.removeListener(listener);
        LOG_INFO("Removed event listener: {}", listener->getListenerName());
    }
}