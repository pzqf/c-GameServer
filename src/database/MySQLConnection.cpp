#include "MySQLConnection.h"
#include "Log.h"
#include <sstream>
#include <algorithm>

MySQLConnection::MySQLConnection(const std::string& host, int port, const std::string& database,
                               const std::string& username, const std::string& password, 
                               int queryTimeout)
    : host_(host), port_(port), database_(database), username_(username), password_(password), queryTimeout_(queryTimeout)
    , connected_(false), inTransaction_(false), affectedRows_(0), columnCount_(0), rowCount_(0) {
}

MySQLConnection::~MySQLConnection() {
    disconnect();
}

std::string MySQLConnection::buildConnectionString() const {
    std::ostringstream connStr;
    connStr << "tcp://" << host_ << ":" << port_ << "/" << database_
            << "?user=" << username_ 
            << "&password=" << password_
            << "&connectTimeout=" << queryTimeout_ * 1000  // MySQL uses milliseconds
            << "&socketTimeout=" << queryTimeout_ * 1000
            << "&default-auth=mysql_native_password";
    return connStr.str();
}

bool MySQLConnection::connect() {
    if (connected_) {
        return true;
    }

    try {
        // Create driver instance
        sql::Driver* driver = sql::mysql::get_driver_instance();
        
        // Build connection URL
        std::string url = buildConnectionString();
        LOG_DEBUG("Connecting to MySQL database: {}:{}", host_, port_);
        
        // Create connection
        connection_.reset(driver->connect(url, username_, password_));
        if (!connection_) {
            setError("Failed to create MySQL connection");
            return false;
        }

        // Set connection properties
        connection_->setAutoCommit(true);
        connection_->setClientOption("OPT_CONNECT_TIMEOUT", std::to_string(queryTimeout_ * 1000).c_str());
        connection_->setClientOption("OPT_READ_TIMEOUT", std::to_string(queryTimeout_ * 1000).c_str());
        connection_->setClientOption("OPT_WRITE_TIMEOUT", std::to_string(queryTimeout_ * 1000).c_str());

        // Set character set to UTF-8
        setCharacterSet("utf8mb4");
        
        // Test connection with ping
        if (!ping()) {
            setError("Connection test failed");
            return false;
        }

        // Create statement
        statement_.reset(connection_->createStatement());
        if (!statement_) {
            setError("Failed to create statement");
            return false;
        }

        connected_ = true;
        LOG_INFO("Successfully connected to MySQL database: {}:{} (DB: {})", host_, port_, database_);
        return true;

    } catch (sql::SQLException& e) {
        setError("MySQL connection failed: " + std::string(e.what()) + 
                " (Error code: " + std::to_string(e.getErrorCode()) + ")");
        return false;
    } catch (const std::exception& e) {
        setError("Exception during MySQL connection: " + std::string(e.what()));
        return false;
    }
}

void MySQLConnection::disconnect() {
    if (!connected_) {
        return;
    }

    // Rollback any pending transaction
    if (inTransaction_) {
        rollbackTransaction();
    }

    // Cleanup result set
    if (resultSet_) {
        resultSet_.reset();
    }

    // Cleanup statement
    if (statement_) {
        statement_.reset();
    }

    // Close connection
    if (connection_) {
        try {
            connection_->close();
        } catch (sql::SQLException& e) {
            LOG_WARN("Error closing MySQL connection: {}", e.what());
        }
        connection_.reset();
    }

    connected_ = false;
    LOG_INFO("Disconnected from MySQL database: {}:{}", host_, port_);
}

bool MySQLConnection::isConnected() const {
    if (!connected_ || !connection_) {
        return false;
    }

    try {
        return connection_->isValid();
    } catch (sql::SQLException& e) {
        LOG_WARN("Connection validation failed: {}", e.what());
        return false;
    }
}

bool MySQLConnection::ping() {
    if (!connected_) {
        return false;
    }

    try {
        return connection_->isValid();
    } catch (sql::SQLException& e) {
        LOG_WARN("Ping failed: {}", e.what());
        return false;
    }
}

bool MySQLConnection::executeQuery(const std::string& query) {
    if (!connected_) {
        setError("Not connected to database");
        return false;
    }

    try {
        cleanup();

        // Execute query
        resultSet_.reset(statement_->executeQuery(query));
        if (!resultSet_) {
            setError("Query execution failed - no result set returned");
            return false;
        }

        // Get column count
        sql::ResultSetMetaData* metadata = resultSet_->getMetaData();
        if (metadata) {
            columnCount_ = metadata->getColumnCount();
        } else {
            columnCount_ = 0;
        }

        LOG_DEBUG("Query executed successfully: {}", query.substr(0, 100) + (query.length() > 100 ? "..." : ""));
        return true;

    } catch (sql::SQLException& e) {
        setError("Query execution failed: " + std::string(e.what()) + 
                " (Error code: " + std::to_string(e.getErrorCode()) + ")");
        return false;
    } catch (const std::exception& e) {
        setError("Exception during query execution: " + std::string(e.what()));
        return false;
    }
}

bool MySQLConnection::executeUpdate(const std::string& query) {
    if (!connected_) {
        setError("Not connected to database");
        return false;
    }

    try {
        cleanup();

        // Execute update
        affectedRows_ = statement_->executeUpdate(query);
        
        LOG_DEBUG("Update executed successfully, affected rows: {}", affectedRows_);
        return true;

    } catch (sql::SQLException& e) {
        setError("Update execution failed: " + std::string(e.what()) + 
                " (Error code: " + std::to_string(e.getErrorCode()) + ")");
        return false;
    } catch (const std::exception& e) {
        setError("Exception during update execution: " + std::string(e.what()));
        return false;
    }
}

bool MySQLConnection::fetchNext() {
    if (!connected_ || !resultSet_) {
        return false;
    }

    try {
        bool hasNext = resultSet_->next();
        if (!hasNext) {
            return false;
        }

        // Reset column data storage
        columnData_.clear();
        columnNull_.clear();
        columnData_.reserve(columnCount_);
        columnNull_.reserve(columnCount_);

        // Store current row data
        for (int i = 1; i <= columnCount_; ++i) {
            if (resultSet_->isNull(i)) {
                columnData_.push_back("");
                columnNull_.push_back(true);
            } else {
                columnData_.push_back(resultSet_->getString(i));
                columnNull_.push_back(false);
            }
        }

        rowCount_++;
        return true;

    } catch (sql::SQLException& e) {
        setError("Fetch failed: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        setError("Exception during fetch: " + std::string(e.what()));
        return false;
    }
}

bool MySQLConnection::getValue(int columnIndex, std::string& value) {
    if (columnIndex < 0 || columnIndex >= static_cast<int>(columnData_.size())) {
        return false;
    }

    if (columnNull_[columnIndex]) {
        value.clear();
        return true;
    }

    value = columnData_[columnIndex];
    return true;
}

bool MySQLConnection::getValue(int columnIndex, int& value) {
    std::string strValue;
    if (!getValue(columnIndex, strValue)) {
        return false;
    }

    if (strValue.empty()) {
        return false;
    }

    try {
        value = std::stoi(strValue);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool MySQLConnection::getValue(int columnIndex, double& value) {
    std::string strValue;
    if (!getValue(columnIndex, strValue)) {
        return false;
    }

    if (strValue.empty()) {
        return false;
    }

    try {
        value = std::stod(strValue);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool MySQLConnection::getValue(int columnIndex, long& value) {
    std::string strValue;
    if (!getValue(columnIndex, strValue)) {
        return false;
    }

    if (strValue.empty()) {
        return false;
    }

    try {
        value = std::stol(strValue);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

sql::PreparedStatement* MySQLConnection::prepareStatement(const std::string& sql) {
    if (!connected_ || !connection_) {
        return nullptr;
    }

    try {
        return connection_->prepareStatement(sql);
    } catch (sql::SQLException& e) {
        setError("Failed to prepare statement: " + std::string(e.what()));
        return nullptr;
    }
}

void MySQLConnection::setAutoCommit(bool autoCommit) {
    if (connected_ && connection_) {
        try {
            connection_->setAutoCommit(autoCommit);
            LOG_DEBUG("AutoCommit set to: {}", autoCommit);
        } catch (sql::SQLException& e) {
            LOG_ERROR("Failed to set auto-commit: {}", e.what());
        }
    }
}

void MySQLConnection::setQueryTimeout(int seconds) {
    queryTimeout_ = seconds;
    if (statement_ && connected_) {
        try {
            statement_->setQueryTimeout(seconds);
        } catch (sql::SQLException& e) {
            LOG_ERROR("Failed to set query timeout: {}", e.what());
        }
    }
}

void MySQLConnection::setCharacterSet(const std::string& charset) {
    if (connected_ && connection_) {
        try {
            connection_->setClientOption("characterSetResults", charset.c_str());
            LOG_DEBUG("Character set set to: {}", charset);
        } catch (sql::SQLException& e) {
            LOG_WARN("Failed to set character set: {}", e.what());
        }
    }
}

void MySQLConnection::setTimezone(const std::string& timezone) {
    if (connected_ && connection_) {
        try {
            connection_->setClientOption("time_zone", timezone.c_str());
            LOG_DEBUG("Timezone set to: {}", timezone);
        } catch (sql::SQLException& e) {
            LOG_WARN("Failed to set timezone: {}", e.what());
        }
    }
}

void MySQLConnection::beginTransaction() {
    if (connected_ && !inTransaction_) {
        try {
            connection_->setAutoCommit(false);
            inTransaction_ = true;
            LOG_DEBUG("Transaction started");
        } catch (sql::SQLException& e) {
            LOG_ERROR("Failed to begin transaction: {}", e.what());
        }
    }
}

void MySQLConnection::commitTransaction() {
    if (connected_ && inTransaction_) {
        try {
            connection_->commit();
            connection_->setAutoCommit(true);
            inTransaction_ = false;
            LOG_DEBUG("Transaction committed");
        } catch (sql::SQLException& e) {
            LOG_ERROR("Failed to commit transaction: {}", e.what());
        }
    }
}

void MySQLConnection::rollbackTransaction() {
    if (connected_ && inTransaction_) {
        try {
            connection_->rollback();
            connection_->setAutoCommit(true);
            inTransaction_ = false;
            LOG_DEBUG("Transaction rolled back");
        } catch (sql::SQLException& e) {
            LOG_ERROR("Failed to rollback transaction: {}", e.what());
        }
    }
}

void MySQLConnection::cleanup() {
    // Cleanup result set
    if (resultSet_) {
        resultSet_.reset();
    }

    // Reset state
    affectedRows_ = 0;
    columnCount_ = 0;
    rowCount_ = 0;
    columnData_.clear();
    columnNull_.clear();
}

void MySQLConnection::setError(const std::string& error) {
    lastError_ = error;
    LOG_ERROR("MySQL connection error: {}", error);
}