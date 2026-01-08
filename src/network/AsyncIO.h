#pragma once

// 跨平台socket类型定义 - 直接在这里定义以避免include问题
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef _WINSOCKAPI_
        #define _WINSOCKAPI_
    #endif
    
    #include <winsock2.h>
    #include <windows.h>
    
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #define SOCKET_ERROR_VAL SOCKET_ERROR
    #define CLOSE_SOCKET(sock) closesocket(sock)
    #define GET_LAST_ERROR() std::to_string(WSAGetLastError())
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    
    typedef int socket_t;
    #define INVALID_SOCKET_VALUE -1
    #define SOCKET_ERROR_VAL -1
    #define CLOSE_SOCKET(sock) close(sock)
    #define GET_LAST_ERROR() std::string(strerror(errno))
#endif

#include <functional>
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include "Log.h"

// Async I/O event types
enum class IOEventType : uint32_t {
    ACCEPT = 0x01,
    READ = 0x02,
    WRITE = 0x04,
    IOERROR = 0x08
};

// Overload bitwise OR operator for IOEventType
inline IOEventType operator|(IOEventType lhs, IOEventType rhs) {
    return static_cast<IOEventType>(
        static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)
    );
}

// I/O event structure
struct IOEvent {
    socket_t socket;
    IOEventType eventType;
    std::string data;
    std::function<void(const IOEvent&)> callback;
};

// Async I/O manager interface
class AsyncIO {
public:
    using EventCallback = std::function<void(const IOEvent&)>;
    
    AsyncIO() = default;
    virtual ~AsyncIO() = default;
    
    // Disable copy and move
    AsyncIO(const AsyncIO&) = delete;
    AsyncIO& operator=(const AsyncIO&) = delete;
    AsyncIO(AsyncIO&&) = delete;
    AsyncIO& operator=(AsyncIO&&) = delete;
    
    // Initialization and cleanup
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    
    // Socket management
    virtual bool addSocket(socket_t socket, IOEventType events) = 0;
    virtual bool removeSocket(socket_t socket) = 0;
    virtual bool modifySocket(socket_t socket, IOEventType events) = 0;
    
    // Async operations
    virtual bool asyncRead(socket_t socket, const std::string& buffer, EventCallback callback) = 0;
    virtual bool asyncWrite(socket_t socket, const std::string& data, EventCallback callback) = 0;
    virtual bool asyncAccept(socket_t serverSocket, EventCallback callback) = 0;
    
    // Event loop
    virtual bool startEventLoop() = 0;
    virtual void stopEventLoop() = 0;
    virtual bool isEventLoopRunning() const = 0;
    
    // Status queries
    virtual int getActiveConnections() const = 0;
    virtual bool isSocketRegistered(socket_t socket) const = 0;
};

// Async I/O manager factory
class AsyncIOFactory {
public:
    static std::unique_ptr<AsyncIO> createAsyncIO();
};

// Cross-platform async I/O manager
class AsyncIOManager {
public:
    using EventCallback = std::function<void(const IOEvent&)>;
    
    AsyncIOManager();
    ~AsyncIOManager();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Socket management
    bool addSocket(socket_t socket, IOEventType events);
    bool removeSocket(socket_t socket);
    bool modifySocket(socket_t socket, IOEventType events);
    
    // Client management
    bool addClient(socket_t socket);
    bool removeClient(socket_t socket);
    bool modifyClient(socket_t socket, IOEventType events);
    
    // Async operations
    bool asyncRead(socket_t socket, const std::string& buffer, EventCallback callback);
    bool asyncWrite(socket_t socket, const std::string& data, EventCallback callback);
    bool asyncAccept(socket_t serverSocket, EventCallback callback);
    
    bool startAsyncRead(socket_t socket, EventCallback callback);
    bool startAsyncWrite(socket_t socket, const std::string& data, EventCallback callback);
    bool startAsyncAccept(socket_t serverSocket, EventCallback callback);
    
    // Event loop control
    bool startEventLoop();
    void stopEventLoop();
    bool isRunning() const { return isRunning_.load(); }
    
    // Set callbacks
    void setAcceptCallback(EventCallback callback) { acceptCallback_ = callback; }
    void setReadCallback(EventCallback callback) { readCallback_ = callback; }
    void setWriteCallback(EventCallback callback) { writeCallback_ = callback; }
    void setErrorCallback(EventCallback callback) { errorCallback_ = callback; }
    
    // Status queries
    int getActiveConnections() const;
    
private:
    void eventLoop();
    
    std::unique_ptr<AsyncIO> asyncIO_;
    std::atomic<bool> isRunning_{false};
    std::thread eventLoopThread_;
    std::mutex mutex_;
    std::condition_variable condition_;
    
    // Callback functions
    EventCallback acceptCallback_;
    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    
    // Active connection count
    std::atomic<int> activeConnections_{0};
};