#include "LinuxEpoll.h"

#ifdef __linux__

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <cerrno>
#include <algorithm>

LinuxEpoll::LinuxEpoll() 
    : epollFd_(-1), initialized_(false), running_(false) {
}

LinuxEpoll::~LinuxEpoll() {
    shutdown();
}

bool LinuxEpoll::initialize() {
    if (initialized_) {
        return true;
    }
    
    // 创建epoll实例
    epollFd_ = epoll_create1(0);
    if (epollFd_ == -1) {
        LOG_ERROR("Failed to create epoll instance: {}", strerror(errno));
        return false;
    }
    
    initialized_ = true;
    LOG_INFO("Linux epoll initialized successfully, fd: {}", epollFd_);
    return true;
}

void LinuxEpoll::shutdown() {
    if (!initialized_) {
        return;
    }
    
    running_ = false;
    
    // 清理所有socket上下文
    {
        std::lock_guard<std::mutex> lock(contextsMutex_);
        socketContexts_.clear();
    }
    
    // 关闭epoll fd
    if (epollFd_ != -1) {
        close(epollFd_);
        epollFd_ = -1;
    }
    
    initialized_ = false;
    LOG_INFO("Linux epoll shutdown completed");
}

bool LinuxEpoll::addSocket(socket_t socket, IOEventType events) {
    if (!initialized_) {
        LOG_ERROR("Epoll not initialized");
        return false;
    }
    
    // 设置socket为非阻塞模式
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        LOG_ERROR("Failed to get socket flags: {}", strerror(errno));
        return false;
    }
    
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_ERROR("Failed to set socket non-blocking: {}", strerror(errno));
        return false;
    }
    
    // 创建socket上下文
    auto context = std::make_unique<SocketContext>();
    context->socket = socket;
    context->events = events;
    
    // 添加到epoll
    struct epoll_event ev;
    ev.events = 0;
    if (events & IOEventType::READ) {
        ev.events |= EPOLLIN;
    }
    if (events & IOEventType::WRITE) {
        ev.events |= EPOLLOUT;
    }
    ev.data.fd = socket;
    
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, socket, &ev) == -1) {
        LOG_ERROR("Failed to add socket to epoll: {}", strerror(errno));
        return false;
    }
    
    // 保存上下文
    {
        std::lock_guard<std::mutex> lock(contextsMutex_);
        socketContexts_[socket] = std::move(context);
    }
    
    LOG_INFO("Socket {} added to epoll with events: {}", socket, static_cast<int>(events));
    return true;
}

bool LinuxEpoll::removeSocket(socket_t socket) {
    if (!initialized_) {
        return false;
    }
    
    // 从epoll中移除
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, socket, nullptr) == -1) {
        LOG_ERROR("Failed to remove socket from epoll: {}", strerror(errno));
        return false;
    }
    
    // 清理上下文
    {
        std::lock_guard<std::mutex> lock(contextsMutex_);
        socketContexts_.erase(socket);
    }
    
    LOG_INFO("Socket {} removed from epoll", socket);
    return true;
}

bool LinuxEpoll::modifySocket(socket_t socket, IOEventType events) {
    if (!initialized_) {
        return false;
    }
    
    struct epoll_event ev;
    ev.events = 0;
    if (events & IOEventType::READ) {
        ev.events |= EPOLLIN;
    }
    if (events & IOEventType::WRITE) {
        ev.events |= EPOLLOUT;
    }
    ev.data.fd = socket;
    
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, socket, &ev) == -1) {
        LOG_ERROR("Failed to modify socket in epoll: {}", strerror(errno));
        return false;
    }
    
    // 更新上下文中的事件
    {
        std::lock_guard<std::mutex> lock(contextsMutex_);
        auto it = socketContexts_.find(socket);
        if (it != socketContexts_.end()) {
            it->second->events = events;
        }
    }
    
    return true;
}

bool LinuxEpoll::asyncRead(socket_t socket, const std::string& buffer, EventCallback callback) {
    // 对于epoll，读操作通过事件触发处理
    // 这里主要是注册回调函数
    std::lock_guard<std::mutex> lock(contextsMutex_);
    auto it = socketContexts_.find(socket);
    if (it != socketContexts_.end()) {
        it->second->callback = callback;
        return true;
    }
    return false;
}

bool LinuxEpoll::asyncWrite(socket_t socket, const std::string& data, EventCallback callback) {
    // 对于epoll，写操作通过事件触发处理
    std::lock_guard<std::mutex> lock(contextsMutex_);
    auto it = socketContexts_.find(socket);
    if (it != socketContexts_.end()) {
        it->second->callback = callback;
        it->second->writeBuffer = data;
        return true;
    }
    return false;
}

bool LinuxEpoll::asyncAccept(socket_t serverSocket, EventCallback callback) {
    std::lock_guard<std::mutex> lock(contextsMutex_);
    auto it = socketContexts_.find(serverSocket);
    if (it != socketContexts_.end()) {
        it->second->callback = callback;
        return true;
    }
    return false;
}

bool LinuxEpoll::startEventLoop() {
    if (!initialized_ || running_) {
        return false;
    }
    
    running_ = true;
    LOG_INFO("Starting Linux epoll event loop");
    return true;
}

void LinuxEpoll::stopEventLoop() {
    running_ = false;
    LOG_INFO("Stopping Linux epoll event loop");
}

bool LinuxEpoll::isEventLoopRunning() const {
    return running_;
}

int LinuxEpoll::getActiveConnections() const {
    std::lock_guard<std::mutex> lock(contextsMutex_);
    return static_cast<int>(socketContexts_.size());
}

bool LinuxEpoll::isSocketRegistered(socket_t socket) const {
    std::lock_guard<std::mutex> lock(contextsMutex_);
    return socketContexts_.find(socket) != socketContexts_.end();
}

void LinuxEpoll::processEvents(int epollFd, struct epoll_event* events, int numEvents) {
    for (int i = 0; i < numEvents; ++i) {
        socket_t fd = events[i].data.fd;
        
        // 查找对应的上下文
        std::unique_ptr<SocketContext> context;
        {
            std::lock_guard<std::mutex> lock(contextsMutex_);
            auto it = socketContexts_.find(fd);
            if (it != socketContexts_.end()) {
                context = std::move(it->second);
                socketContexts_.erase(it);
            }
        }
        
        if (!context) {
            continue;
        }
        
        // 处理事件
        if (events[i].events & (EPOLLIN | EPOLLPRI)) {
            handleReadEvent(context.get());
        }
        
        if (events[i].events & EPOLLOUT) {
            handleWriteEvent(context.get());
        }
        
        if (events[i].events & (EPOLLERR | EPOLLHUP)) {
            IOEvent errorEvent{context->socket, IOEventType::IOERROR, "", nullptr};
            if (context->callback) {
                context->callback(errorEvent);
            }
        }
    }
}

void LinuxEpoll::handleReadEvent(SocketContext* context) {
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = recv(context->socket, buffer, sizeof(buffer), 0);
    
    if (bytesRead > 0) {
        context->readBuffer.append(buffer, bytesRead);
        
        IOEvent readEvent{context->socket, IOEventType::READ, context->readBuffer, context->callback};
        if (context->callback) {
            context->callback(readEvent);
        }
    } else if (bytesRead == 0) {
        // 连接关闭
        IOEvent errorEvent{context->socket, IOEventType::IOERROR, "Connection closed", context->callback};
        if (context->callback) {
            context->callback(errorEvent);
        }
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("Read error on socket {}: {}", context->socket, strerror(errno));
            IOEvent errorEvent{context->socket, IOEventType::IOERROR, strerror(errno), context->callback};
            if (context->callback) {
                context->callback(errorEvent);
            }
        }
    }
}

void LinuxEpoll::handleWriteEvent(SocketContext* context) {
    if (context->writeBuffer.empty()) {
        return;
    }
    
    ssize_t bytesSent = send(context->socket, context->writeBuffer.data() + context->writeOffset, 
                           context->writeBuffer.size() - context->writeOffset, 0);
    
    if (bytesSent > 0) {
        context->writeOffset += bytesSent;
        
        if (context->writeOffset >= context->writeBuffer.size()) {
            // 发送完成
            IOEvent writeEvent{context->socket, IOEventType::WRITE, context->writeBuffer, context->callback};
            if (context->callback) {
                context->callback(writeEvent);
            }
            context->writeBuffer.clear();
            context->writeOffset = 0;
        }
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("Write error on socket {}: {}", context->socket, strerror(errno));
            IOEvent errorEvent{context->socket, IOEventType::IOERROR, strerror(errno), context->callback};
            if (context->callback) {
                context->callback(errorEvent);
            }
        }
    }
}

void LinuxEpoll::handleAcceptEvent(socket_t serverSocket) {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    socket_t clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
    if (clientSocket >= 0) {
        IOEvent acceptEvent{clientSocket, IOEventType::READ, "", nullptr};
        // 这里需要通过回调处理accept事件
        LOG_INFO("Accepted new connection from socket {}", clientSocket);
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("Accept error: {}", strerror(errno));
        }
    }
}

#endif // __linux__