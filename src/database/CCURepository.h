#pragma once

#include <string>
#include <vector>

// CCU信息结构体
struct CCUInfo {
    int id;
    std::string name;
    std::string status;
    int concurrent_users;
    std::string created_at;
    std::string updated_at;
    
    // 通用方法用于获取ID
    int getId() const { return id; }
};

// CCU仓储类 - 简化版本
class CCURepository {
public:
    explicit CCURepository() = default;
    
    // 静态方法：通用的实体映射
    static void mapRowToEntity(const std::map<std::string, std::string>& row, CCUInfo& ccu) {
        try {
            if (row.count("id")) ccu.id = std::stoi(row.at("id"));
            if (row.count("name")) ccu.name = row.at("name");
            if (row.count("status")) ccu.status = row.at("status");
            if (row.count("concurrent_users")) ccu.concurrent_users = std::stoi(row.at("concurrent_users"));
            if (row.count("created_at")) ccu.created_at = row.at("created_at");
            if (row.count("updated_at")) ccu.updated_at = row.at("updated_at");
        } catch (const std::exception& e) {
            // 静默处理映射错误
            (void)e;
        }
    }
    
    // 获取CCU信息通过名称 - 简化版本
    bool getByName(const std::string& name, CCUInfo& ccu) {
        // 临时实现 - 返回false表示未找到
        (void)name;
        (void)ccu;
        return false;
    }
    
    // 获取CCU信息通过ID - 简化版本
    bool getById(int id, CCUInfo& ccu) {
        // 临时实现 - 返回false表示未找到
        (void)id;
        (void)ccu;
        return false;
    }
    
    // 创建CCU - 简化版本
    bool create(const CCUInfo& ccu) {
        // 临时实现 - 返回true表示成功
        (void)ccu;
        return true;
    }
    
    // 更新CCU - 简化版本
    bool update(const CCUInfo& ccu) {
        // 临时实现 - 返回true表示成功
        (void)ccu;
        return true;
    }
    
    // 删除CCU - 简化版本
    bool deleteById(int id) {
        // 临时实现 - 返回true表示成功
        (void)id;
        return true;
    }
    
    // 更新并发用户数 - 简化版本
    bool updateConcurrentUsers(const std::string& name, int concurrentUsers) {
        // 临时实现 - 返回true表示成功
        (void)name;
        (void)concurrentUsers;
        return true;
    }
    
    // 增加并发用户数 - 简化版本
    bool incrementConcurrentUsers(const std::string& name) {
        // 临时实现 - 返回true表示成功
        (void)name;
        return true;
    }
    
    // 减少并发用户数 - 简化版本
    bool decrementConcurrentUsers(const std::string& name) {
        // 临时实现 - 返回true表示成功
        (void)name;
        return true;
    }
    
    // 获取总CCU数量 - 简化版本
    int getTotalCount() {
        return 0;
    }
    
    // 获取活跃CCU数量 - 简化版本
    int getActiveCount() {
        return 0;
    }
    
    // 获取总并发用户数 - 简化版本
    int getTotalConcurrentUsers() {
        return 0;
    }
    
    // 获取所有CCU（分页）- 简化版本
    std::vector<CCUInfo> getAll(int limit = 100, int offset = 0) {
        // 临时实现 - 返回空向量
        (void)limit;
        (void)offset;
        return std::vector<CCUInfo>();
    }
    
    // 获取活跃CCU列表（分页）- 简化版本
    std::vector<CCUInfo> getActive(int limit = 100, int offset = 0) {
        // 临时实现 - 返回空向量
        (void)limit;
        (void)offset;
        return std::vector<CCUInfo>();
    }
};

// 为了向后兼容，保留原有的CCUDB类作为包装器
class CCUDB {
private:
    CCURepository repository_;
    
public:
    static CCUDB& getInstance() {
        static CCUDB instance;
        return instance;
    }
    
    // 兼容原有接口
    bool getCCUByName(const std::string& name, CCUInfo& ccu) {
        return repository_.getByName(name, ccu);
    }
    
    bool getCCUById(int id, CCUInfo& ccu) {
        return repository_.getById(id, ccu);
    }
    
    bool createCCU(const CCUInfo& ccu) {
        return repository_.create(ccu);
    }
    
    bool updateCCU(const CCUInfo& ccu) {
        return repository_.update(ccu);
    }
    
    bool deleteCCU(int id) {
        return repository_.deleteById(id);
    }
    
    bool updateConcurrentUsers(const std::string& name, int concurrentUsers) {
        return repository_.updateConcurrentUsers(name, concurrentUsers);
    }
    
    bool incrementConcurrentUsers(const std::string& name) {
        return repository_.incrementConcurrentUsers(name);
    }
    
    bool decrementConcurrentUsers(const std::string& name) {
        return repository_.decrementConcurrentUsers(name);
    }
    
    int getTotalCount() {
        return repository_.getTotalCount();
    }
    
    int getActiveCount() {
        return repository_.getActiveCount();
    }
    
    int getTotalConcurrentUsers() {
        return repository_.getTotalConcurrentUsers();
    }
    
    std::vector<CCUInfo> getAllCCU(int limit = 100, int offset = 0) {
        return repository_.getAll(limit, offset);
    }
    
    std::vector<CCUInfo> getActiveCCU(int limit = 100, int offset = 0) {
        return repository_.getActive(limit, offset);
    }
    
private:
    CCUDB() = default;
};