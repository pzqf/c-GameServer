#include "MySQLPool.h"
#include "Log.h"
#include <algorithm>

MySQLPool::MySQLPool(const Config& config)
    : config_(config), activeConnections_(0), totalConnections_(0), 
      shutdown_(false), currentPoolSize_(0) {
}

MySQLPool::~MySQLPool() {
    shutdown();
}

bool MySQLPool::initialize() {
    LOG_INFO("Initializing MySQL connection pool...");
    
    if (config_.minPoolSize < 0 || config_.maxPoolSize < 0 || 
        config_.minPoolSize > config_.maxPoolSize) {
        LOG_ERROR("Invalid pool size configuration: min={}, max={}", 
                 config_.minPoolSize, config_.maxPoolSize);
        return false;
    }

    if (config_.minPoolSize > 0) {
        // Pre-create minimum connections
        for (int i = 0; i < config_.minPoolSize; ++i) {
            if (!createConnection()) {
                LOG_ERROR("Failed to create initial connection {}", i + 1);
                return false;
            }
        }
        currentPoolSize_ = config_.minPoolSize;
    }

    // Start cleanup thread
    cleanupThread_ = std::thread(&MySQLPool::cleanupThreadFunc, this);
    
    // Start health check thread  
    healthCheckThread_ = std::thread(&MySQLPool::connectionHealthCheck, this);
    
    LOG_INFO("MySQL connection pool initialized successfully. Pool size: {}/{}", 
             currentPoolSize_, config_.maxPoolSize);
    return true;
}

void MySQLPool::shutdown() {
    if (shutdown_.exchange(true)) {
        return; // Already shutting down
    }

    LOG_INFO("Shutting down MySQL connection pool...");

    // Signal cleanup threads
    condition_.notify_all();

    // Wait for cleanup thread to finish
    if (cleanupThread_.joinable()) {
        cleanupThread_.join();
    }

    // Wait for health check thread to finish
    if (healthCheckThread_.joinable()) {
        healthCheckThread_.join();
    }

    // Close all idle connections
    {
        std::lock_guard<std::mutex> lock(mutex_);
        idleConnections_.clear();
        totalConnections_ = 0;
        activeConnections_ = 0;
        currentPoolSize_ = 0;
    }

    LOG_INFO("MySQL connection pool shutdown completed");
}

std::unique_ptr<MySQLConnection> MySQLPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait for available connection or timeout
    std::chrono::seconds timeout(config_.connectionTimeout);
    auto deadline = std::chrono::steady_clock::now() + timeout;
    
    while (!shutdown_ && idleConnections_.empty() && canCreateNewConnection()) {
        // Try to create a new connection if pool can grow
        if (createConnection()) {
            break;
        }
        
        // Wait for a connection to become available
        if (condition_.wait_until(lock, deadline) == std::cv_status::timeout) {
            LOG_ERROR("Failed to acquire connection within timeout");
            return nullptr;
        }
    }

    if (shutdown_ || idleConnections_.empty()) {
        return nullptr;
    }

    // Get connection from pool
    auto connection = std::move(idleConnections_.back());
    idleConnections_.pop_back();
    
    activeConnections_++;
    
    LOG_DEBUG("Acquired connection. Active: {}, Idle: {}", 
             activeConnections_.load(), idleConnections_.size());
    
    return connection;
}

void MySQLPool::release(std::unique_ptr<MySQLConnection> conn) {
    if (!conn) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    if (shutdown_) {
        // Pool is shutting down, just discard the connection
        LOG_DEBUG("Connection discarded during shutdown");
        return;
    }

    // Check if connection is still healthy
    if (!isConnectionHealthy(conn)) {
        LOG_DEBUG("Releasing unhealthy connection");
        totalConnections_--;
        if (currentPoolSize_ > 0) {
            currentPoolSize_--;
        }
        return;
    }

    // Add to idle pool if within size limits
    if (idleConnections_.size() < static_cast<size_t>(currentPoolSize_)) {
        idleConnections_.push_back(std::move(conn));
        lastConnectionReleaseTime_ = std::chrono::steady_clock::now();
    } else {
        // Pool is at capacity, discard connection
        totalConnections_--;
        if (currentPoolSize_ > 0) {
            currentPoolSize_--;
        }
        LOG_DEBUG("Connection discarded - pool at capacity");
    }

    activeConnections_--;
    
    LOG_DEBUG("Released connection. Active: {}, Idle: {}", 
             activeConnections_.load(), idleConnections_.size());
    
    condition_.notify_one();
}

bool MySQLPool::createConnection() {
    if (!canCreateNewConnection()) {
        return false;
    }

    try {
        auto connection = std::make_unique<MySQLConnection>(
            config_.host, config_.port, config_.database,
            config_.username, config_.password, config_.queryTimeout);
            
        if (connection->connect()) {
            idleConnections_.push_back(std::move(connection));
            totalConnections_++;
            activeConnections_++;
            
            LOG_DEBUG("Created new connection. Total: {}, Active: {}, Idle: {}", 
                     totalConnections_.load(), activeConnections_.load(), idleConnections_.size());
            return true;
        } else {
            LOG_ERROR("Failed to connect new connection: {}", connection->getErrorMessage());
            return false;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception creating connection: {}", e.what());
        return false;
    }
}

void MySQLPool::cleanupConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto currentTime = std::chrono::steady_clock::now();
    
    // Remove idle connections that have been idle too long
    auto it = idleConnections_.begin();
    while (it != idleConnections_.end()) {
        if (currentPoolSize_ > config_.minPoolSize && 
            idleConnections_.size() > static_cast<size_t>(config_.minPoolSize)) {
            
            // Connection has been idle too long, remove it
            it = idleConnections_.erase(it);
            totalConnections_--;
            currentPoolSize_--;
            
            LOG_DEBUG("Cleaned up idle connection. Pool size: {}", currentPoolSize_);
        } else {
            ++it;
        }
    }
}

void MySQLPool::cleanupIdleConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto currentTime = std::chrono::steady_clock::now();
    auto idleTime = std::chrono::duration_cast<std::chrono::seconds>(
        currentTime - lastConnectionReleaseTime_);
    
    // Remove connections that have been idle too long
    if (idleTime > connectionMaxIdleTime_ && 
        currentPoolSize_ > config_.minPoolSize) {
        
        // Remove one idle connection
        if (!idleConnections_.empty()) {
            idleConnections_.pop_back();
            totalConnections_--;
            currentPoolSize_--;
            
            LOG_DEBUG("Removed idle connection due to timeout. Pool size: {}", currentPoolSize_);
        }
    }
}

void MySQLPool::connectionHealthCheck() {
    while (!shutdown_) {
        std::this_thread::sleep_for(healthCheckInterval_);
        
        if (shutdown_) break;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check all idle connections for health
        for (auto& conn : idleConnections_) {
            if (!isConnectionHealthy(conn)) {
                LOG_DEBUG("Found unhealthy connection during health check");
                replaceUnhealthyConnection(conn);
            }
        }
    }
}

void MySQLPool::cleanupThreadFunc() {
    while (!shutdown_) {
        std::this_thread::sleep_for(std::chrono::seconds(60)); // Run every minute
        
        if (shutdown_) break;
        
        cleanupIdleConnections();
    }
}

bool MySQLPool::canCreateNewConnection() const {
    return totalConnections_.load() < config_.maxPoolSize && 
           activeConnections_.load() < config_.maxPoolSize;
}

bool MySQLPool::isConnectionHealthy(std::unique_ptr<MySQLConnection>& conn) {
    if (!conn || !conn->isConnected()) {
        return false;
    }

    // Test connection with a simple ping
    try {
        return conn->ping();
    } catch (const std::exception& e) {
        LOG_WARN("Connection health check failed: {}", e.what());
        return false;
    }
}

void MySQLPool::replaceUnhealthyConnection(std::unique_ptr<MySQLConnection>& conn) {
    try {
        // Remove unhealthy connection
        conn.reset();
        totalConnections_--;
        if (currentPoolSize_ > 0) {
            currentPoolSize_--;
        }

        // Try to create a replacement if we can
        if (canCreateNewConnection()) {
            createConnection();
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error replacing unhealthy connection: {}", e.what());
    }
}

bool MySQLPool::setPoolSize(int newSize) {
    if (newSize < config_.minPoolSize || newSize > config_.maxPoolSize) {
        LOG_ERROR("Invalid pool size {} - must be between {} and {}", 
                 newSize, config_.minPoolSize, config_.maxPoolSize);
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    resizePool(newSize);
    
    LOG_INFO("Pool size changed to {}", newSize);
    return true;
}

bool MySQLPool::increasePoolSize(int increment) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int newSize = currentPoolSize_ + increment;
    if (newSize > config_.maxPoolSize) {
        LOG_ERROR("Cannot increase pool size to {} - max is {}", newSize, config_.maxPoolSize);
        return false;
    }
    
    resizePool(newSize);
    return true;
}

bool MySQLPool::decreasePoolSize(int decrement) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int newSize = currentPoolSize_ - decrement;
    if (newSize < config_.minPoolSize) {
        LOG_ERROR("Cannot decrease pool size to {} - min is {}", newSize, config_.minPoolSize);
        return false;
    }
    
    resizePool(newSize);
    return true;
}

void MySQLPool::resizePool(int newSize) {
    if (newSize == currentPoolSize_) {
        return;
    }
    
    if (newSize > currentPoolSize_) {
        // Need to create more connections
        int toCreate = newSize - currentPoolSize_;
        for (int i = 0; i < toCreate; ++i) {
            if (!createConnection()) {
                LOG_ERROR("Failed to create connection during resize");
                break;
            }
        }
    } else {
        // Need to remove connections
        int toRemove = currentPoolSize_ - newSize;
        for (int i = 0; i < toRemove && !idleConnections_.empty(); ++i) {
            idleConnections_.pop_back();
            totalConnections_--;
        }
    }
    
    currentPoolSize_ = newSize;
    LOG_INFO("Pool resized to {} connections", currentPoolSize_);
}