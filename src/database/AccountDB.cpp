#include "AccountDB.h"
#include "logging/Log.h"
#include "messaging/result.h"
#include <sstream>
#include <algorithm>
#include <future>

AccountDB::AccountDB() : accountRepository_() {
    // 初始化账号仓储实例
}

AccountDB& AccountDB::getInstance() {
    static AccountDB instance;
    return instance;
}

bool AccountDB::getAccountByUsername(const std::string& username, AccountInfo& account) {
    return accountRepository_.getByUsername(username, account);
}

bool AccountDB::getAccountById(int id, AccountInfo& account) {
    return accountRepository_.getById(id, account);
}

bool AccountDB::createAccount(const AccountInfo& account) {
    return accountRepository_.create(account);
}

bool AccountDB::updateAccount(const AccountInfo& account) {
    return accountRepository_.update(account);
}

bool AccountDB::deleteAccount(int id) {
    return accountRepository_.deleteById(id);
}

bool AccountDB::verifyPassword(const std::string& username, const std::string& password) {
    AccountInfo account;
    if (getAccountByUsername(username, account)) {
        return account.password == password && account.status == "active";
    }
    return false;
}

bool AccountDB::updateLastLogin(const std::string& username) {
    AccountInfo account;
    if (getAccountByUsername(username, account)) {
        account.updated_at = "NOW()";
        return updateAccount(account);
    }
    return false;
}

std::vector<AccountInfo> AccountDB::getAllAccounts(int limit, int offset) {
    return accountRepository_.getAll(limit, offset);
}

// 异步方法实现
std::future<OperationResultPtr> AccountDB::asyncGetAccountByUsername(const std::string& username) {
    return std::async(std::launch::async, &AccountDB::performGetAccountByUsername, this, username);
}

std::future<OperationResultPtr> AccountDB::asyncCreateAccount(const AccountInfo& account) {
    return std::async(std::launch::async, &AccountDB::performCreateAccount, this, account);
}

std::future<OperationResultPtr> AccountDB::asyncVerifyPassword(const std::string& username, const std::string& password) {
    return std::async(std::launch::async, &AccountDB::performVerifyPassword, this, username, password);
}

OperationResultPtr AccountDB::performGetAccountByUsername(const std::string& username) {
    auto result = std::make_unique<OperationResult>(1, ResponseType::SUCCESS, "Account retrieved successfully");
    
    try {
        AccountInfo account;
        if (getAccountByUsername(username, account)) {
            std::map<std::string, std::string> accountData;
            accountData["id"] = std::to_string(account.id);
            accountData["username"] = account.username;
            accountData["password"] = account.password;
            accountData["email"] = account.email;
            accountData["status"] = account.status;
            accountData["created_at"] = account.created_at;
            accountData["updated_at"] = account.updated_at;
            
            std::vector<std::map<std::string, std::string>> resultData;
            resultData.push_back(accountData);
            std::stringstream ss;
            for (const auto& row : resultData) {
                for (const auto& [key, value] : row) {
                    ss << key << ":" << value << ";";
                }
                ss << "|";
            }
            result->setData(ss.str());
        } else {
            result = std::make_unique<OperationResult>(1, ResponseType::NOT_FOUND, "Account not found");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error in asyncGetAccountByUsername: {}", e.what());
        result = std::make_unique<OperationResult>(1, ResponseType::DATABASE_ERROR, std::string("Database error: ") + e.what());
    }
    
    return result;
}

OperationResultPtr AccountDB::performCreateAccount(const AccountInfo& account) {
    auto result = std::make_unique<OperationResult>(1, ResponseType::SUCCESS, "Account created successfully");
    
    try {
        if (createAccount(account)) {
            result->setData(std::string("Account created successfully"));
        } else {
            result = std::make_unique<OperationResult>(1, ResponseType::SERVICE_ERROR, "Failed to create account");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error in asyncCreateAccount: {}", e.what());
        result = std::make_unique<OperationResult>(1, ResponseType::DATABASE_ERROR, std::string("Database error: ") + e.what());
    }
    
    return result;
}

OperationResultPtr AccountDB::performVerifyPassword(const std::string& username, const std::string& password) {
    auto result = std::make_unique<OperationResult>(1, ResponseType::SUCCESS, "Password verification successful");
    
    try {
        bool isValid = verifyPassword(username, password);
        result->setData(isValid ? "true" : "false");
    } catch (const std::exception& e) {
        LOG_ERROR("Error in asyncVerifyPassword: {}", e.what());
        result = std::make_unique<OperationResult>(1, ResponseType::DATABASE_ERROR, std::string("Database error: ") + e.what());
    }
    
    return result;
}