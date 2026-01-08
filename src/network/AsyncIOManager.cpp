#include "AsyncIO.h"
#include "LinuxEpoll.h"
#include "WindowsIOCP.h"

#include <memory>

// AsyncIOFactory实现
std::unique_ptr<AsyncIO> AsyncIOFactory::createAsyncIO() {
#ifdef _WIN32
    return std::make_unique<WindowsIOCP>();
#else
    return std::make_unique<LinuxEpoll>();
#endif
}

AsyncIOManager::AsyncIOManager()
    : isRunning_(false), asyncIO_(nullptr),
      acceptCallback_(nullptr), readCallback_(nullptr), 
      writeCallback_(nullptr), errorCallback_(nullptr)
{
}

AsyncIOManager::~AsyncIOManager()
{
    shutdown();
}

bool AsyncIOManager::initialize()
{
    // 创建平台特定的异步I/O实现
    asyncIO_ = AsyncIOFactory::createAsyncIO();
    if (!asyncIO_) {
        LOG_ERROR("Failed to create async I/O implementation");
        return false;
    }
    
    // 初始化异步I/O实现
    if (!asyncIO_->initialize()) {
        LOG_ERROR("Failed to initialize async I/O implementation");
        asyncIO_.reset();
        return false;
    }
    
    LOG_INFO("AsyncIOManager initialized successfully");
    return true;
}

void AsyncIOManager::shutdown()
{
    // 停止事件循环
    if (isRunning_.load()) {
        stopEventLoop();
    }
    
    // 关闭异步I/O实现
    if (asyncIO_) {
        asyncIO_->shutdown();
        asyncIO_.reset();
    }
    
    LOG_INFO("AsyncIOManager shutdown complete");
}

bool AsyncIOManager::addClient(socket_t socket)
{
    if (!asyncIO_) {
        LOG_ERROR("AsyncIOManager not initialized");
        return false;
    }
    
    // 注册socket用于读和错误事件
    bool success = asyncIO_->addSocket(socket, IOEventType::READ | IOEventType::IOERROR);
    if (success) {
        activeConnections_++;
        LOG_DEBUG("Socket {} added to async I/O manager", socket);
    } else {
        LOG_ERROR("Failed to add socket {} to async I/O manager", socket);
    }
    
    return success;
}

bool AsyncIOManager::addSocket(socket_t socket, IOEventType events)
{
    if (!asyncIO_) {
        LOG_ERROR("AsyncIOManager not initialized");
        return false;
    }
    
    // 注册socket用于指定的事件类型
    bool success = asyncIO_->addSocket(socket, events);
    if (success) {
        activeConnections_++;
        LOG_DEBUG("Socket {} added to async I/O manager", socket);
    } else {
        LOG_ERROR("Failed to add socket {} to async I/O manager", socket);
    }
    
    return success;
}

bool AsyncIOManager::removeClient(socket_t socket)
{
    if (!asyncIO_) {
        LOG_ERROR("AsyncIOManager not initialized");
        return false;
    }
    
    bool success = asyncIO_->removeSocket(socket);
    if (success) {
        activeConnections_--;
        LOG_DEBUG("Socket {} removed from async I/O manager", socket);
    } else {
        LOG_ERROR("Failed to remove socket {} from async I/O manager", socket);
    }
    
    return success;
}

bool AsyncIOManager::modifyClient(socket_t socket, IOEventType events)
{
    if (!asyncIO_) {
        LOG_ERROR("AsyncIOManager not initialized");
        return false;
    }
    
    return asyncIO_->modifySocket(socket, events);
}

// 异步I/O操作的别名方法
bool AsyncIOManager::asyncRead(socket_t socket, const std::string& buffer, EventCallback callback)
{
    return startAsyncRead(socket, callback);
}

bool AsyncIOManager::asyncWrite(socket_t socket, const std::string& data, EventCallback callback)
{
    return startAsyncWrite(socket, data, callback);
}

bool AsyncIOManager::asyncAccept(socket_t serverSocket, EventCallback callback)
{
    return startAsyncAccept(serverSocket, callback);
}

bool AsyncIOManager::startAsyncRead(socket_t socket, EventCallback callback)
{
    if (!asyncIO_) {
        LOG_ERROR("AsyncIOManager not initialized");
        return false;
    }
    
    return asyncIO_->asyncRead(socket, "", callback);
}

bool AsyncIOManager::startAsyncWrite(socket_t socket, const std::string& data, EventCallback callback)
{
    if (!asyncIO_) {
        LOG_ERROR("AsyncIOManager not initialized");
        return false;
    }
    
    return asyncIO_->asyncWrite(socket, data, callback);
}

bool AsyncIOManager::startAsyncAccept(socket_t serverSocket, EventCallback callback)
{
    if (!asyncIO_) {
        LOG_ERROR("AsyncIOManager not initialized");
        return false;
    }
    
    return asyncIO_->asyncAccept(serverSocket, callback);
}

bool AsyncIOManager::startEventLoop()
{
    if (!asyncIO_) {
        LOG_ERROR("AsyncIOManager not initialized");
        return false;
    }
    
    if (isRunning_.load()) {
        LOG_WARN("Event loop is already running");
        return true;
    }
    
    // 启动异步I/O的事件循环
    if (!asyncIO_->startEventLoop()) {
        LOG_ERROR("Failed to start async I/O event loop");
        return false;
    }
    
    isRunning_ = true;
    
    // 启动管理线程来处理事件分发
    eventLoopThread_ = std::thread(&AsyncIOManager::eventLoop, this);
    
    LOG_INFO("AsyncIOManager event loop started");
    return true;
}

void AsyncIOManager::stopEventLoop()
{
    if (!isRunning_.load()) {
        LOG_WARN("Event loop is not running");
        return;
    }
    
    // 停止异步I/O事件循环
    if (asyncIO_) {
        asyncIO_->stopEventLoop();
    }
    
    isRunning_ = false;
    
    // 等待事件循环线程结束
    if (eventLoopThread_.joinable()) {
        eventLoopThread_.join();
    }
    
    LOG_INFO("AsyncIOManager event loop stopped");
}

int AsyncIOManager::getActiveConnections() const
{
    return activeConnections_.load();
}

void AsyncIOManager::eventLoop()
{
    LOG_INFO("AsyncIOManager event loop thread started");
    
    while (isRunning_.load()) {
        // 检查是否有活跃连接
        int activeConnections = getActiveConnections();
        if (activeConnections == 0) {
            // 没有活跃连接时休眠一段时间
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        // 在实际实现中，这里会检查异步I/O的状态
        // 并调用相应的回调函数
        
        // 短暂休眠以避免过度占用CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    LOG_INFO("AsyncIOManager event loop thread ended");
}