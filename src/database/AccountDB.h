#pragma once

#include <string>
#include <vector>
#include <memory>
#include <future>
#include <mutex>
#include "AccountRepository.h"
#include "messaging/result.h"

class AccountDB {
public:
    static AccountDB& getInstance();

    bool getAccountByUsername(const std::string& username, AccountInfo& account);
    bool getAccountById(int id, AccountInfo& account);
    bool createAccount(const AccountInfo& account);
    bool updateAccount(const AccountInfo& account);
    bool deleteAccount(int id);
    bool verifyPassword(const std::string& username, const std::string& password);
    bool updateLastLogin(const std::string& username);
    
    std::vector<AccountInfo> getAllAccounts(int limit = 100, int offset = 0);

    std::future<OperationResultPtr> asyncGetAccountByUsername(const std::string& username);
    std::future<OperationResultPtr> asyncCreateAccount(const AccountInfo& account);
    std::future<OperationResultPtr> asyncVerifyPassword(const std::string& username, const std::string& password);

private:
    AccountDB();
    ~AccountDB() = default;

    OperationResultPtr performGetAccountByUsername(const std::string& username);
    OperationResultPtr performCreateAccount(const AccountInfo& account);
    OperationResultPtr performVerifyPassword(const std::string& username, const std::string& password);
    
    AccountRepository accountRepository_;
};