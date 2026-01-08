#pragma once

#include "SocketTypes.h"
#include "AsyncIO.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCKAPI_
    #define _WINSOCKAPI_
#endif

#include <windows.h>
#include <mswsock.h>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <thread>
#include <mutex>

// 异步操作类型
enum class AsyncOperationType {
    READ,
    WRITE,
    ACCEPT
};

// Windows IOCP实现
class WindowsIOCP : public AsyncIO {
public:
    WindowsIOCP();
    ~WindowsIOCP() override;
    
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
    struct OverlappedEx;
    
    struct SocketContext {
        socket_t socket;
        IOEventType events;
        EventCallback callback;
        std::string readBuffer;
        std::string writeBuffer;
        size_t readOffset;
        size_t writeOffset;
        OverlappedEx* readOverlapped;
        OverlappedEx* writeOverlapped;
        SOCKET listenSocket;
    };
    
    struct OverlappedEx {
        OVERLAPPED overlapped;
        socket_t socket;
        AsyncOperationType operation;
        std::string buffer;
        size_t bufferSize;
        size_t bytesTransferred;
        SocketContext* context;
    };
    
    static constexpr int MAX_THREADS = 4;  // 固定线程数，避免constexpr问题
    static constexpr int BUFFER_SIZE = 4096;
    static constexpr int MAX_POSTED_RECEIVES = 10;
    
    HANDLE iocpHandle_;
    bool initialized_;
    bool running_;
    std::vector<std::thread> workerThreads_;
    
    std::unordered_map<socket_t, std::unique_ptr<SocketContext>> socketContexts_;
    mutable std::mutex contextsMutex_;
    
    // 完成端口工作线程
    void workerThread();
    
    // 处理完成通知
    void processCompletion(DWORD bytesTransferred, ULONG_PTR completionKey, LPOVERLAPPED overlapped);
    
    // 异步操作
    bool postRead(SocketContext* context);
    bool postWrite(SocketContext* context);
    bool postAccept(SocketContext* context);
    
    // 完成键到socket的转换
    socket_t completionKeyToSocket(ULONG_PTR completionKey);
};

#endif // _WIN32