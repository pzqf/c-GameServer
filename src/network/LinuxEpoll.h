#pragma once

#include "SocketTypes.h"
#include "AsyncIO.h"

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

// Linux epoll实现
class LinuxEpoll : public AsyncIO {
public:
    LinuxEpoll();
    ~LinuxEpoll() override;
    
    // AsyncIO 接口实现
    bool initialize() override;
    void shutdown() override;
    
    bool addSocket(socket_t socket, IOEventType events) override;
    bool removeSocket(socket_t socket) override;
    bool modifySocket(socket_t socket, IOEventType events) override;
    
    bool asyncRead(socket_t socket, const std::string& buffer, EventCallback callback) override;
    bool asyncWrite(socket_t socket, const std::string& data, EventCallback callback) override;
    bool asyncAccept(socket_t serverSocket, EventCallback callback) override;
    
    bool startEventLoop() override;
    void stopEventLoop() override;
    bool isEventLoopRunning() const override;
    
    int getActiveConnections() const override;
    bool isSocketRegistered(socket_t socket) const override;
    
private:
    struct SocketContext {
        socket_t socket;
        IOEventType events;
        EventCallback callback;
        std::string readBuffer;
        std::string writeBuffer;
        size_t readOffset;
        size_t writeOffset;
    };
    
    void processEvents(int epollFd, struct epoll_event* events, int numEvents);
    void handleReadEvent(SocketContext* context);
    void handleWriteEvent(SocketContext* context);
    void handleAcceptEvent(socket_t serverSocket);
    
    static constexpr int MAX_EVENTS = 64;
    static constexpr int BUFFER_SIZE = 4096;
    
    int epollFd_;
    bool initialized_;
    bool running_;
    
    std::unordered_map<socket_t, std::unique_ptr<SocketContext>> socketContexts_;
    mutable std::mutex contextsMutex_;
};

#endif // __linux__