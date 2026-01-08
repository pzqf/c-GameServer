#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <atomic>
#include <chrono>
#include <functional>
#include "MySQLConnection.h"

class MySQLPool {
public:
    struct Config {
        std::string host;
        int port;
        std::string database;
        std::string username;
        std::string password;
        int connectionTimeout;
        int queryTimeout;
        int minPoolSize;
        int maxPoolSize;
        int idleTimeout; // seconds
    };

    explicit MySQLPool(const Config& config);
    ~MySQLPool();

    MySQLPool(const MySQLPool&) = delete;
    MySQLPool& operator=(const MySQLPool&) = delete;
    MySQLPool(MySQLPool&&) = delete;
    MySQLPool& operator=(MySQLPool&&) = delete;

    bool initialize();
    void shutdown();

    // Get connection from pool
    std::unique_ptr<MySQLConnection> acquire();
    // Release connection back to pool
    void release(std::unique_ptr<MySQLConnection> conn);

    // Pool statistics
    int getActiveConnections() const { return activeConnections_.load(); }
    size_t getIdleConnections() const { return idleConnections_.size(); }
    int getTotalConnections() const { return totalConnections_.load(); }
    
    // Dynamic pool management
    bool setPoolSize(int newSize);
    int getPoolSize() const { return currentPoolSize_; }
    bool increasePoolSize(int increment);
    bool decreasePoolSize(int decrement);
    void resizePool(int newSize);

private:
    bool createConnection();
    void cleanupConnections();
    void cleanupIdleConnections();
    void connectionHealthCheck();
    void cleanupThreadFunc();
    
    // Helper methods
    bool canCreateNewConnection() const;
    bool isConnectionHealthy(std::unique_ptr<MySQLConnection>& conn);
    void replaceUnhealthyConnection(std::unique_ptr<MySQLConnection>& conn);

    Config config_;
    std::vector<std::unique_ptr<MySQLConnection>> idleConnections_;
    std::queue<std::unique_ptr<MySQLConnection>> busyConnections_;
    std::mutex mutex_;
    std::condition_variable condition_;
    
    // Statistics and state tracking
    std::atomic<int> activeConnections_{0};
    std::atomic<int> totalConnections_{0};
    std::atomic<bool> shutdown_{false};
    std::thread cleanupThread_;
    std::thread healthCheckThread_;
    
    // Connection lifecycle management
    std::chrono::steady_clock::time_point lastConnectionReleaseTime_;
    int currentPoolSize_;
    
    // Health check settings
    std::chrono::seconds healthCheckInterval_{30}; // Check every 30 seconds
    std::chrono::seconds connectionMaxIdleTime_{300}; // 5 minutes max idle
    std::chrono::seconds connectionMaxLifetime_{3600}; // 1 hour max lifetime
};