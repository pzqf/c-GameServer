#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "MySQLConnection.h"
#include "MySQLPool.h"

// 前向声明
class ConfigManager;

class DatabaseManager {
public:
    static DatabaseManager& getInstance();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    DatabaseManager(DatabaseManager&&) = delete;
    DatabaseManager& operator=(DatabaseManager&&) = delete;

    bool initialize(ConfigManager* configManager = nullptr);
    void shutdown();

    std::unique_ptr<MySQLConnection> getConnection(const std::string& database);
    
    void returnConnection(const std::string& database, std::unique_ptr<MySQLConnection> conn);
    
    // Get pool status
    int getPoolSize(const std::string& database) const;
    int getActiveConnections(const std::string& database) const;
    int getIdleConnections(const std::string& database) const;

    // Dynamic pool size management
    bool setPoolSize(const std::string& database, int newSize);
    bool increasePoolSize(const std::string& database, int increment);
    bool decreasePoolSize(const std::string& database, int decrement);
    void resizePool(const std::string& database, int newSize);

private:
    DatabaseManager() = default;
    ~DatabaseManager() = default;

    void createPools(ConfigManager* configManager);

    std::unordered_map<std::string, std::unique_ptr<MySQLPool>> pools_;
    bool initialized_;
};