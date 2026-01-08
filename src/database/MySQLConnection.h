#pragma once

#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <mysql/jdbc.h>

class MySQLConnection {
public:
    MySQLConnection(const std::string& host, int port, const std::string& database,
                   const std::string& username, const std::string& password, 
                   int queryTimeout = 60);
    ~MySQLConnection();

    MySQLConnection(const MySQLConnection&) = delete;
    MySQLConnection& operator=(const MySQLConnection&) = delete;
    MySQLConnection(MySQLConnection&&) = delete;
    MySQLConnection& operator=(MySQLConnection&&) = delete;

    bool connect();
    void disconnect();
    bool isConnected() const;

    bool executeQuery(const std::string& query);
    bool executeUpdate(const std::string& query);
    
    bool fetchNext();
    bool getValue(int columnIndex, std::string& value);
    bool getValue(int columnIndex, int& value);
    bool getValue(int columnIndex, double& value);
    bool getValue(int columnIndex, long& value);
    
    int getAffectedRows() const { return affectedRows_; }
    int getColumnCount() const { return columnCount_; }
    int getRowCount() const { return rowCount_; }
    
    std::string getErrorMessage() const { return lastError_; }
    
    void beginTransaction();
    void commitTransaction();
    void rollbackTransaction();
    bool inTransaction() const { return inTransaction_; }

    // MySQL specific methods
    sql::PreparedStatement* prepareStatement(const std::string& sql);
    void setAutoCommit(bool autoCommit);
    void setQueryTimeout(int seconds);
    
    // Connection management
    bool ping();
    void setCharacterSet(const std::string& charset);
    void setTimezone(const std::string& timezone);

private:
    void cleanup();
    void setError(const std::string& error);
    std::string buildConnectionString() const;

    std::string host_;
    int port_;
    std::string database_;
    std::string username_;
    std::string password_;
    int queryTimeout_;
    
    std::unique_ptr<sql::Connection> connection_;
    std::unique_ptr<sql::Statement> statement_;
    std::unique_ptr<sql::ResultSet> resultSet_;
    
    bool connected_;
    bool inTransaction_;
    int affectedRows_;
    int columnCount_;
    int rowCount_;
    std::string lastError_;
    
    // Column data storage for result sets
    std::vector<std::string> columnData_;
    std::vector<bool> columnNull_;
};