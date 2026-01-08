#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "spdlog/spdlog.h"

// 包含数据库相关头文件
#include "../src/database/MySQLConnection.h"
#include "../src/database/MySQLPool.h"
#include "../src/database/DatabaseManager.h"
#include "../src/database/AccountRepository.h"
#include "../src/database/CCURepository.h"
#include "../src/config/ConfigManager.h"
#include "../src/database/DatabaseRepository.h"

class MySQLTest {
private:
    std::unique_ptr<AccountRepository> accountRepo;
    std::unique_ptr<CCURepository> ccuRepo;

public:
    MySQLTest() {
        // 初始化日志
        spdlog::set_level(spdlog::level::info);
        spdlog::info("=== MySQL Database Connection Test ===");
    }

    ~MySQLTest() {
        // DatabaseManager是单例，会在程序结束时自动清理
    }

    bool initialize() {
        try {
            spdlog::info("Initializing database manager...");
            
            // 获取数据库管理器单例实例
            DatabaseManager& manager = DatabaseManager::getInstance();
            
            // 初始化数据库管理器（会读取配置文件）
            if (!manager.initialize()) {
                spdlog::error("Failed to initialize database manager");
                return false;
            }
            
            spdlog::info("Database manager initialized successfully");
            
            // 创建仓库实例
            accountRepo = std::make_unique<AccountRepository>();
            ccuRepo = std::make_unique<CCURepository>();
            
            return true;
        } catch (const std::exception& e) {
            spdlog::error("Exception during initialization: {}", e.what());
            return false;
        }
    }

    void testConnection() {
        spdlog::info("\n=== Testing Database Connections ===");
        
        try {
            DatabaseManager& manager = DatabaseManager::getInstance();
            
            // 测试Account数据库连接
            spdlog::info("Testing account database connection...");
            auto accountConn = manager.getConnection("account");
            if (accountConn && accountConn->isConnected()) {
                spdlog::info("✓ Account database connection successful");
                
                // 执行简单的查询测试
                if (accountConn->executeQuery("SELECT 1")) {
                    spdlog::info("✓ Account database query test successful");
                } else {
                    spdlog::error("✗ Account database query test failed: {}", accountConn->getErrorMessage());
                }
                
                manager.returnConnection("account", std::move(accountConn));
            } else {
                spdlog::error("✗ Account database connection failed");
            }
            
            // 测试Game数据库连接
            spdlog::info("Testing game database connection...");
            auto gameConn = manager.getConnection("game");
            if (gameConn && gameConn->isConnected()) {
                spdlog::info("✓ Game database connection successful");
                
                // 执行简单的查询测试
                if (gameConn->executeQuery("SELECT 1")) {
                    spdlog::info("✓ Game database query test successful");
                } else {
                    spdlog::error("✗ Game database query test failed: {}", gameConn->getErrorMessage());
                }
                
                manager.returnConnection("game", std::move(gameConn));
            } else {
                spdlog::error("✗ Game database connection failed");
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Exception during connection test: {}", e.what());
        }
    }

    void testConnectionPool() {
        spdlog::info("\n=== Testing Connection Pool ===");
        
        try {
            DatabaseManager& manager = DatabaseManager::getInstance();
            
            // 获取连接池统计信息
            int accountActive = manager.getActiveConnections("account");
            int accountIdle = manager.getIdleConnections("account");
            int accountTotal = manager.getPoolSize("account");
            
            int gameActive = manager.getActiveConnections("game");
            int gameIdle = manager.getIdleConnections("game");
            int gameTotal = manager.getPoolSize("game");
            
            spdlog::info("Account database pool stats:");
            spdlog::info("  Active connections: {}", accountActive);
            spdlog::info("  Idle connections: {}", accountIdle);
            spdlog::info("  Total connections: {}", accountTotal);
            
            spdlog::info("Game database pool stats:");
            spdlog::info("  Active connections: {}", gameActive);
            spdlog::info("  Idle connections: {}", gameIdle);
            spdlog::info("  Total connections: {}", gameTotal);
            
            // 测试多个并发连接
            spdlog::info("Testing multiple concurrent connections...");
            std::vector<std::unique_ptr<MySQLConnection>> connections;
            
            for (int i = 0; i < 5; i++) {
                auto conn = manager.getConnection("account");
                if (conn && conn->isConnected()) {
                    connections.push_back(std::move(conn));
                    spdlog::info("  Connection {} acquired", i + 1);
                } else {
                    spdlog::error("  Failed to acquire connection {}", i + 1);
                    break;
                }
            }
            
            spdlog::info("Testing concurrent queries...");
            for (size_t i = 0; i < connections.size(); i++) {
                if (connections[i]->executeQuery("SELECT CONNECTION_ID() as conn_id, NOW() as current_time")) {
                    spdlog::info("  Query {} executed successfully", i + 1);
                } else {
                    spdlog::error("  Query {} failed: {}", i + 1, connections[i]->getErrorMessage());
                }
            }
            
            // 释放连接
            spdlog::info("Releasing connections...");
            for (size_t i = 0; i < connections.size(); i++) {
                manager.returnConnection("account", std::move(connections[i]));
            }
            
            spdlog::info("✓ Connection pool test completed");
            
        } catch (const std::exception& e) {
            spdlog::error("Exception during connection pool test: {}", e.what());
        }
    }

    void testAccountRepository() {
        spdlog::info("\n=== Testing Account Repository ===");
        
        try {
            // 测试获取不存在的账号（应该返回false）
            AccountInfo account;
            spdlog::info("Testing getByUsername with non-existent user...");
            if (accountRepo->getByUsername("non_existent_user_12345", account)) {
                spdlog::warn("? Unexpectedly found non-existent user");
            } else {
                spdlog::info("✓ Correctly returned false for non-existent user");
            }
            
            // 如果有测试数据，可以测试获取存在的账号
            // 这里我们只是测试方法调用不会崩溃
            spdlog::info("Account repository test completed (no data verification)");
            
        } catch (const std::exception& e) {
            spdlog::error("Exception during account repository test: {}", e.what());
        }
    }

    void testCCURepository() {
        spdlog::info("\n=== Testing CCU Repository ===");
        
        try {
            // 测试获取不存在的CCU服务器（应该返回false）
            CCUInfo ccu;
            spdlog::info("Testing getByName with non-existent CCU...");
            if (ccuRepo->getByName("non_existent_ccu_12345", ccu)) {
                spdlog::warn("? Unexpectedly found non-existent CCU");
            } else {
                spdlog::info("✓ Correctly returned false for non-existent CCU");
            }
            
            // 测试统计方法
            spdlog::info("Testing CCU statistics...");
            int totalCount = ccuRepo->getTotalCount();
            int activeCount = ccuRepo->getActiveCount();
            int totalUsers = ccuRepo->getTotalConcurrentUsers();
            
            spdlog::info("  Total CCU count: {}", totalCount);
            spdlog::info("  Active CCU count: {}", activeCount);
            spdlog::info("  Total concurrent users: {}", totalUsers);
            
            spdlog::info("✓ CCU repository test completed");
            
        } catch (const std::exception& e) {
            spdlog::error("Exception during CCU repository test: {}", e.what());
        }
    }

    void testPerformance() {
        spdlog::info("\n=== Testing Performance ===");
        
        try {
            DatabaseManager& manager = DatabaseManager::getInstance();
            
            const int testIterations = 100;
            spdlog::info("Running {} connection and query iterations...", testIterations);
            
            auto startTime = std::chrono::high_resolution_clock::now();
            
            for (int i = 0; i < testIterations; i++) {
                // 获取连接
                auto conn = manager.getConnection("account");
                if (!conn || !conn->isConnected()) {
                    spdlog::error("Failed to get connection at iteration {}", i);
                    break;
                }
                
                // 执行简单查询
                std::string query = "SELECT " + std::to_string(i) + " as test_value, NOW() as test_time";
                if (!conn->executeQuery(query)) {
                    spdlog::error("Query failed at iteration {}: {}", i, conn->getErrorMessage());
                    break;
                }
                
                // 释放连接
                manager.returnConnection("account", std::move(conn));
                
                // 每10次迭代显示进度
                if ((i + 1) % 10 == 0) {
                    spdlog::info("  Completed {} iterations", i + 1);
                }
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            spdlog::info("✓ Performance test completed");
            spdlog::info("  Total time: {} ms", duration.count());
            spdlog::info("  Average per iteration: {:.2f} ms", 
                        static_cast<double>(duration.count()) / testIterations);
            
        } catch (const std::exception& e) {
            spdlog::error("Exception during performance test: {}", e.what());
        }
    }

    void runAllTests() {
        spdlog::info("Starting comprehensive MySQL database tests...");
        
        if (!initialize()) {
            spdlog::error("Failed to initialize. Exiting.");
            return;
        }
        
        testConnection();
        testConnectionPool();
        testAccountRepository();
        testCCURepository();
        testPerformance();
        
        spdlog::info("\n=== All Tests Completed ===");
        
        // 显示最终的连接池状态
        DatabaseManager& manager = DatabaseManager::getInstance();
        int finalActive = manager.getActiveConnections("account");
        int finalIdle = manager.getIdleConnections("account");
        int finalTotal = manager.getPoolSize("account");
        
        spdlog::info("Final connection pool status:");
        spdlog::info("  Active: {}, Idle: {}, Total: {}", 
                    finalActive, finalIdle, finalTotal);
    }
};

int main(int argc, char* argv[]) {
    try {
        MySQLTest test;
        test.runAllTests();
        
        spdlog::info("MySQL database test completed successfully.");
        return 0;
        
    } catch (const std::exception& e) {
        spdlog::error("Fatal exception: {}", e.what());
        return 1;
    }
}