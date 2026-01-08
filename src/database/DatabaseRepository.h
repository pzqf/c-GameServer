#pragma once

#include <string>
#include <vector>
#include <map>

namespace Database {

// 数据库操作结果枚举
enum DatabaseStatus {
    DBS_SUCCESS = 0,
    DBS_ERROR = 1,
    DBS_NOT_FOUND = 2
};

// 数据库操作结果类
class DatabaseResult {
private:
    DatabaseStatus status_;
    std::string message_;
    std::vector<std::map<std::string, std::string>> data_;

public:
    DatabaseResult() 
        : status_(DBS_SUCCESS), 
          message_("") {
    }
    
    DatabaseResult(DatabaseStatus status, const std::string& msg) 
        : status_(status), 
          message_(msg) {
    }
    
    DatabaseStatus getStatus() const {
        return status_;
    }
    
    const std::string& getMessage() const {
        return message_;
    }
    
    void setStatus(DatabaseStatus status) {
        status_ = status;
    }
    
    void setMessage(const std::string& msg) {
        message_ = msg;
    }
    
    const std::vector<std::map<std::string, std::string>>& getData() const {
        return data_;
    }
    
    void setData(const std::vector<std::map<std::string, std::string>>& data) {
        data_ = data;
    }
    
    bool isSuccess() const {
        return status_ == DBS_SUCCESS;
    }
    
    bool hasData() const {
        return !data_.empty();
    }
};

// 数据库仓库基类
class DatabaseRepository {
protected:
    std::string tableName_;
    
public:
    explicit DatabaseRepository(const std::string& tableName) 
        : tableName_(tableName) {
    }
    
    virtual ~DatabaseRepository() {
    }
    
protected:
    template<typename EntityType>
    static void mapRowToEntity(const std::map<std::string, std::string>& row, EntityType& entity) {
        (void)row;
        (void)entity;
    }
};

} // namespace Database