#include "WindowsIOCP.h"

#ifdef _WIN32

#include <windows.h>
#include <mswsock.h>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

WindowsIOCP::WindowsIOCP() 
    : iocpHandle_(INVALID_HANDLE_VALUE), initialized_(false), running_(false) {
}

WindowsIOCP::~WindowsIOCP() {
    shutdown();
}

bool WindowsIOCP::initialize() {
    if (initialized_) {
        return true;
    }
    
    // 创建I/O完成端口
    iocpHandle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (iocpHandle_ == NULL) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to create IOCP: {}", error);
        return false;
    }
    
    initialized_ = true;
    LOG_INFO("Windows IOCP initialized successfully, handle: {}", iocpHandle_);
    return true;
}

void WindowsIOCP::shutdown() {
    if (!initialized_) {
        return;
    }
    
    running_ = false;
    
    // 停止所有工作线程
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    workerThreads_.clear();
    
    // 关闭所有socket
    {
        std::lock_guard<std::mutex> lock(contextsMutex_);
        for (auto& pair : socketContexts_) {
            if (pair.second->socket != INVALID_SOCKET_VALUE) {
                CLOSE_SOCKET(pair.second->socket);
            }
        }
        socketContexts_.clear();
    }
    
    // 关闭IOCP句柄
    if (iocpHandle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(iocpHandle_);
        iocpHandle_ = INVALID_HANDLE_VALUE;
    }
    
    initialized_ = false;
    LOG_INFO("Windows IOCP shutdown completed");
}

bool WindowsIOCP::addSocket(socket_t socket, IOEventType events) {
    if (!initialized_) {
        LOG_ERROR("IOCP not initialized");
        return false;
    }
    
    // 设置socket为重叠模式
    DWORD bytesReturned = 0;
    // Windows IOCP 模式下不需要特殊的socket设置
    
    // 创建socket上下文
    auto context = std::make_unique<SocketContext>();
    context->socket = socket;
    context->events = events;
    context->readOverlapped = nullptr;
    context->writeOverlapped = nullptr;
    
    // 将socket关联到IOCP
    HANDLE resultHandle = CreateIoCompletionPort((HANDLE)socket, iocpHandle_, (ULONG_PTR)socket, 0);
    if (resultHandle == NULL) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to associate socket with IOCP: {}", error);
        return false;
    }
    
    // 保存上下文
    {
        std::lock_guard<std::mutex> lock(contextsMutex_);
        socketContexts_[socket] = std::move(context);
    }
    
    LOG_INFO("Socket {} added to IOCP with events: {}", socket, static_cast<int>(events));
    return true;
}

bool WindowsIOCP::removeSocket(socket_t socket) {
    if (!initialized_) {
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(contextsMutex_);
        socketContexts_.erase(socket);
    }
    
    LOG_INFO("Socket {} removed from IOCP", socket);
    return true;
}

bool WindowsIOCP::modifySocket(socket_t socket, IOEventType events) {
    if (!initialized_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(contextsMutex_);
    auto it = socketContexts_.find(socket);
    if (it != socketContexts_.end()) {
        it->second->events = events;
        return true;
    }
    
    return false;
}

bool WindowsIOCP::asyncRead(socket_t socket, const std::string& buffer, EventCallback callback) {
    std::lock_guard<std::mutex> lock(contextsMutex_);
    auto it = socketContexts_.find(socket);
    if (it != socketContexts_.end()) {
        it->second->callback = callback;
        return postRead(it->second.get());
    }
    return false;
}

bool WindowsIOCP::asyncWrite(socket_t socket, const std::string& data, EventCallback callback) {
    std::lock_guard<std::mutex> lock(contextsMutex_);
    auto it = socketContexts_.find(socket);
    if (it != socketContexts_.end()) {
        it->second->callback = callback;
        it->second->writeBuffer = data;
        return postWrite(it->second.get());
    }
    return false;
}

bool WindowsIOCP::asyncAccept(socket_t serverSocket, EventCallback callback) {
    std::lock_guard<std::mutex> lock(contextsMutex_);
    auto it = socketContexts_.find(serverSocket);
    if (it != socketContexts_.end()) {
        it->second->callback = callback;
        return postAccept(it->second.get());
    }
    return false;
}

bool WindowsIOCP::startEventLoop() {
    if (!initialized_ || running_) {
        return false;
    }
    
    running_ = true;
    
    // 启动工作线程
    for (int i = 0; i < MAX_THREADS; ++i) {
        workerThreads_.emplace_back(&WindowsIOCP::workerThread, this);
    }
    
    LOG_INFO("Starting Windows IOCP event loop with {} threads", MAX_THREADS);
    return true;
}

void WindowsIOCP::stopEventLoop() {
    running_ = false;
    
    // 发送关闭通知到所有工作线程
    for (int i = 0; i < MAX_THREADS; ++i) {
        PostQueuedCompletionStatus(iocpHandle_, 0, NULL, NULL);
    }
    
    LOG_INFO("Stopping Windows IOCP event loop");
}

bool WindowsIOCP::isEventLoopRunning() const {
    return running_;
}

int WindowsIOCP::getActiveConnections() const {
    std::lock_guard<std::mutex> lock(contextsMutex_);
    return static_cast<int>(socketContexts_.size());
}

bool WindowsIOCP::isSocketRegistered(socket_t socket) const {
    std::lock_guard<std::mutex> lock(contextsMutex_);
    return socketContexts_.find(socket) != socketContexts_.end();
}

void WindowsIOCP::workerThread() {
    while (running_) {
        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0;
        LPOVERLAPPED overlapped = nullptr;
        
        // 等待完成通知
        BOOL result = GetQueuedCompletionStatus(iocpHandle_, &bytesTransferred, &completionKey, &overlapped, INFINITE);
        
        if (!running_) {
            break;
        }
        
        if (result == FALSE && overlapped == NULL) {
            // 错误或超时
            DWORD error = GetLastError();
            if (error != WAIT_TIMEOUT) {
                LOG_ERROR("GetQueuedCompletionStatus error: {}", error);
            }
            continue;
        }
        
        // 处理完成通知
        processCompletion(bytesTransferred, completionKey, overlapped);
    }
}

void WindowsIOCP::processCompletion(DWORD bytesTransferred, ULONG_PTR completionKey, LPOVERLAPPED overlapped) {
    if (overlapped == NULL) {
        return;
    }
    
    auto overlappedEx = std::unique_ptr<OverlappedEx>(reinterpret_cast<OverlappedEx*>(overlapped));
    socket_t socket = overlappedEx->socket;
    
    std::lock_guard<std::mutex> lock(contextsMutex_);
    auto it = socketContexts_.find(socket);
    if (it == socketContexts_.end()) {
        return;
    }
    
    SocketContext* context = it->second.get();
    
    if (bytesTransferred == 0) {
        // 连接关闭
        IOEvent errorEvent{context->socket, IOEventType::IOERROR, "Connection closed", context->callback};
        if (context->callback) {
            context->callback(errorEvent);
        }
        socketContexts_.erase(it);
        return;
    }
    
    switch (overlappedEx->operation) {
        case AsyncOperationType::READ: {
            context->readBuffer.append(overlappedEx->buffer, bytesTransferred);
            IOEvent readEvent{context->socket, IOEventType::READ, context->readBuffer, context->callback};
            if (context->callback) {
                context->callback(readEvent);
            }
            // 继续读取
            postRead(context);
            break;
        }
            
        case AsyncOperationType::WRITE: {
            context->writeOffset += bytesTransferred;
            if (context->writeOffset >= context->writeBuffer.size()) {
                // 发送完成
                IOEvent writeEvent{context->socket, IOEventType::WRITE, context->writeBuffer, context->callback};
                if (context->callback) {
                    context->callback(writeEvent);
                }
                context->writeBuffer.clear();
                context->writeOffset = 0;
            } else {
                // 继续发送
                postWrite(context);
            }
            break;
        }
            
        case AsyncOperationType::ACCEPT: {
            // 处理accept完成
            IOEvent acceptEvent{overlappedEx->socket, IOEventType::READ, "", context->callback};
            if (context->callback) {
                context->callback(acceptEvent);
            }
            // 继续accept
            postAccept(context);
            break;
        }
    }
    
    // overlappedEx会自动释放
}

bool WindowsIOCP::postRead(SocketContext* context) {
    if (context->readOverlapped) {
        return true; // 已经有一个read操作在进行
    }
    
    auto overlapped = std::make_unique<OverlappedEx>();
    overlapped->socket = context->socket;
    overlapped->operation = AsyncOperationType::READ;
    overlapped->bufferSize = BUFFER_SIZE;
    overlapped->bytesTransferred = 0;
    overlapped->context = context;
    
    auto buffer = std::make_unique<char[]>(BUFFER_SIZE);
    overlapped->buffer.assign(buffer.get(), BUFFER_SIZE);
    
    WSABUF wsabuf;
    wsabuf.buf = buffer.get();
    wsabuf.len = BUFFER_SIZE;
    
    DWORD flags = 0;
    int result = WSARecv(context->socket, &wsabuf, 1, NULL, &flags, &overlapped->overlapped, NULL);
    
    if (result == SOCKET_ERROR) {
        DWORD error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            LOG_ERROR("WSARecv failed: {}", error);
            return false;
        }
    }
    
    context->readOverlapped = overlapped.release();
    return true;
}

bool WindowsIOCP::postWrite(SocketContext* context) {
    if (context->writeOverlapped) {
        return true; // 已经有一个write操作在进行
    }
    
    if (context->writeBuffer.empty()) {
        return true; // 没有数据需要发送
    }
    
    auto overlapped = std::make_unique<OverlappedEx>();
    overlapped->socket = context->socket;
    overlapped->operation = AsyncOperationType::WRITE;
    overlapped->buffer = context->writeBuffer.substr(context->writeOffset);
    overlapped->bufferSize = overlapped->buffer.size();
    overlapped->bytesTransferred = 0;
    overlapped->context = context;
    
    WSABUF wsabuf;
    wsabuf.buf = const_cast<char*>(overlapped->buffer.data());
    wsabuf.len = overlapped->bufferSize;
    
    DWORD bytesSent = 0;
    int result = WSASend(context->socket, &wsabuf, 1, &bytesSent, 0, &overlapped->overlapped, NULL);
    
    if (result == SOCKET_ERROR) {
        DWORD error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            LOG_ERROR("WSASend failed: {}", error);
            return false;
        }
    }
    
    context->writeOverlapped = overlapped.release();
    return true;
}

bool WindowsIOCP::postAccept(SocketContext* context) {
    // 创建一个新的socket用于accept
    SOCKET acceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (acceptSocket == INVALID_SOCKET) {
        LOG_ERROR("Failed to create accept socket: {}", WSAGetLastError());
        return false;
    }
    
    // 创建accept上下文
    auto acceptContext = std::make_unique<SocketContext>();
    acceptContext->socket = acceptSocket;
    acceptContext->events = IOEventType::READ;
    acceptContext->callback = context->callback;
    
    // 预分配缓冲区用于AcceptEx
    const size_t bufferSize = 1024;
    auto buffer = std::make_unique<char[]>(bufferSize * 2);
    
    // 将新socket添加到IOCP
    if (!addSocket(acceptSocket, IOEventType::READ)) {
        CLOSE_SOCKET(acceptSocket);
        return false;
    }
    
    // 保存accept上下文
    {
        std::lock_guard<std::mutex> lock(contextsMutex_);
        socketContexts_[acceptSocket] = std::move(acceptContext);
    }
    
    LOG_INFO("Accept operation posted for server socket {}", context->socket);
    return true;
}

socket_t WindowsIOCP::completionKeyToSocket(ULONG_PTR completionKey) {
    return static_cast<socket_t>(completionKey);
}

#endif // _WIN32