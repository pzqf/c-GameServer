#include "DatabaseManager.h"
#include "MySQLPool.h"
#include "../config/ConfigManager.h"
#include "../logging/Log.h"
#include <algorithm>

DatabaseManager& DatabaseManager::getInstance() {
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::initialize(ConfigManager* configManager) {
    if (initialized_) {
        LOG_INFO("DatabaseManager already initialized");
        return true;
    }

    LOG_INFO("Initializing DatabaseManager...");

    createPools(configManager);

    initialized_ = true;
    LOG_INFO("DatabaseManager initialized successfully");
    return true;
}

void DatabaseManager::shutdown() {
    if (!initialized_) {
        return;
    }

    LOG_INFO("Shutting down DatabaseManager...");

    for (auto& pool : pools_) {
        pool.second->shutdown();
    }

    pools_.clear();
    initialized_ = false;
    
    LOG_INFO("DatabaseManager shutdown complete");
}

std::unique_ptr<MySQLConnection> DatabaseManager::getConnection(const std::string& database) {
    auto pool = pools_.find(database);
    if (pool == pools_.end()) {
        LOG_ERROR("Database pool not found: {}", database);
        return nullptr;
    }

    return pool->second->acquire();
}

void DatabaseManager::returnConnection(const std::string& database, std::unique_ptr<MySQLConnection> conn) {
    auto pool = pools_.find(database);
    if (pool == pools_.end()) {
        LOG_ERROR("Database pool not found: {}", database);
        return;
    }

    pool->second->release(std::move(conn));
}

// Get pool status using string-based interface
int DatabaseManager::getPoolSize(const std::string& database) const {
    auto pool = pools_.find(database);
    return (pool != pools_.end()) ? pool->second->getPoolSize() : 0;
}

int DatabaseManager::getActiveConnections(const std::string& database) const {
    auto pool = pools_.find(database);
    return (pool != pools_.end()) ? pool->second->getActiveConnections() : 0;
}

int DatabaseManager::getIdleConnections(const std::string& database) const {
    auto pool = pools_.find(database);
    return static_cast<int>((pool != pools_.end()) ? pool->second->getIdleConnections() : 0);
}

// Dynamic pool size management using string-based interface
bool DatabaseManager::setPoolSize(const std::string& database, int newSize) {
    auto pool = pools_.find(database);
    if (pool == pools_.end()) {
        LOG_ERROR("Database pool not found: {}", database);
        return false;
    }
    return pool->second->setPoolSize(newSize);
}

bool DatabaseManager::increasePoolSize(const std::string& database, int increment) {
    auto pool = pools_.find(database);
    if (pool == pools_.end()) {
        LOG_ERROR("Database pool not found: {}", database);
        return false;
    }
    return pool->second->increasePoolSize(increment);
}

bool DatabaseManager::decreasePoolSize(const std::string& database, int decrement) {
    auto pool = pools_.find(database);
    if (pool == pools_.end()) {
        LOG_ERROR("Database pool not found: {}", database);
        return false;
    }
    return pool->second->decreasePoolSize(decrement);
}

void DatabaseManager::resizePool(const std::string& database, int newSize) {
    auto pool = pools_.find(database);
    if (pool == pools_.end()) {
        LOG_ERROR("Database pool not found: {}", database);
        return;
    }
    pool->second->resizePool(newSize);
}

void DatabaseManager::createPools(ConfigManager* configManager) {
    if (!configManager) {
        LOG_ERROR("ConfigManager is null");
        return;
    }

    LOG_INFO("Creating MySQL database connection pools using ConfigManager");

    // Create Account database pool
    MySQLPool::Config accountConfig;
    accountConfig.host = configManager->getAccountDBHost();
    accountConfig.port = configManager->getAccountDBPort();
    accountConfig.database = configManager->getAccountDBDatabase();
    accountConfig.username = configManager->getAccountDBUsername();
    accountConfig.password = configManager->getAccountDBPassword();
    accountConfig.connectionTimeout = configManager->getAccountDBConnectionTimeout();
    accountConfig.queryTimeout = configManager->getAccountDBQueryTimeout();
    accountConfig.minPoolSize = configManager->getAccountDBMinPoolSize();
    accountConfig.maxPoolSize = configManager->getAccountDBMaxPoolSize();
    // 设置空闲连接超时时间为300秒
    accountConfig.idleTimeout = 300;

    auto accountPool = std::make_unique<MySQLPool>(accountConfig);
    if (accountPool->initialize()) {
        pools_["account"] = std::move(accountPool);
        LOG_INFO("Account database pool created with min size: {}, max size: {}", 
                 accountConfig.minPoolSize, accountConfig.maxPoolSize);
    } else {
        LOG_ERROR("Failed to create Account database pool");
    }

    // Create Game database pool
    MySQLPool::Config gameConfig;
    gameConfig.host = configManager->getGameDBHost();
    gameConfig.port = configManager->getGameDBPort();
    gameConfig.database = configManager->getGameDBDatabase();
    gameConfig.username = configManager->getGameDBUsername();
    gameConfig.password = configManager->getGameDBPassword();
    gameConfig.connectionTimeout = configManager->getGameDBConnectionTimeout();
    gameConfig.queryTimeout = configManager->getGameDBQueryTimeout();
    gameConfig.minPoolSize = configManager->getGameDBMinPoolSize();
    gameConfig.maxPoolSize = configManager->getGameDBMaxPoolSize();
    // 设置空闲连接超时时间为300秒
    gameConfig.idleTimeout = 300;

    auto gamePool = std::make_unique<MySQLPool>(gameConfig);
    if (gamePool->initialize()) {
        pools_["game"] = std::move(gamePool);
        LOG_INFO("Game database pool created with min size: {}, max size: {}", 
                 gameConfig.minPoolSize, gameConfig.maxPoolSize);
    } else {
        LOG_ERROR("Failed to create Game database pool");
    }
}