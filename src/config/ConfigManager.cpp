#include "ConfigManager.h"
#include <iostream>

ConfigManager::ConfigManager(const std::string& configFile) : reader(nullptr), configFile(configFile)
{
    loadConfig();
}

ConfigManager::~ConfigManager()
{
    if (reader)
    {
        delete reader;
    }
}

void ConfigManager::loadConfig()
{
    if (reader)
    {
        delete reader;
    }
    
    reader = new INIReader(configFile);
    
    if (!reader->isValidFile())
    {
        std::cerr << "Warning: Could not load config file: " << configFile << std::endl;
        std::cerr << "Using default values." << std::endl;
    }
}

// Server Configuration
int ConfigManager::getServerPort() const
{
    return reader ? reader->getInt("Server", "Port", 8080) : 8080;
}

std::string ConfigManager::getServerHost() const
{
    return reader ? reader->getString("Server", "Host", "localhost") : "localhost";
}

std::string ConfigManager::getServerName() const
{
    return reader ? reader->getString("Server", "ServerName", "AccountSvr") : "AccountSvr";
}

int ConfigManager::getMaxConnections() const
{
    return reader ? reader->getInt("Server", "MaxConnections", 1000) : 1000;
}

// Database Configuration
std::string ConfigManager::getDatabaseHost() const
{
    return reader ? reader->getString("Database", "Host", "localhost") : "localhost";
}

int ConfigManager::getDatabasePort() const
{
    return reader ? reader->getInt("Database", "Port", 3306) : 3306;
}

std::string ConfigManager::getDatabaseName() const
{
    return reader ? reader->getString("Database", "Name", "accountsvr") : "accountsvr";
}

std::string ConfigManager::getDatabaseUser() const
{
    return reader ? reader->getString("Database", "User", "root") : "root";
}

std::string ConfigManager::getDatabasePassword() const
{
    return reader ? reader->getString("Database", "Password", "") : "";
}

int ConfigManager::getDatabaseMaxConnections() const
{
    return reader ? reader->getInt("Database", "MaxConnections", 50) : 50;
}

int ConfigManager::getDatabaseConnectionTimeout() const
{
    return reader ? reader->getInt("Database", "ConnectionTimeout", 30) : 30;
}

int ConfigManager::getDatabaseMaxRetries() const
{
    return reader ? reader->getInt("Database", "MaxRetries", 3) : 3;
}

int ConfigManager::getDatabaseRetryDelay() const
{
    return reader ? reader->getInt("Database", "RetryDelay", 5) : 5;
}

// Logging Configuration (Unified)
std::string ConfigManager::getLogLevel() const
{
    return reader ? reader->getString("Logging", "Level", "INFO") : "INFO";
}

std::string ConfigManager::getLogFilePath() const
{
    return reader ? reader->getString("Logging", "FilePath", "logs/accountsvr.log") : "logs/accountsvr.log";
}

bool ConfigManager::isConsoleLogging() const
{
    return reader ? reader->getBool("Logging", "Console", true) : true;
}

bool ConfigManager::isFileLogging() const
{
    return reader ? reader->getBool("Logging", "File", true) : true;
}

std::string ConfigManager::getLogPattern() const
{
    return reader ? reader->getString("Logging", "Pattern", "[%Y-%m-%d %H:%M:%S.%f] [%l] [%t] %v") : "[%Y-%m-%d %H:%M:%S.%f] [%l] [%t] %v";
}

bool ConfigManager::isAsyncLogging() const
{
    return reader ? reader->getBool("Logging", "Async", true) : true;
}

int ConfigManager::getAsyncThreadCount() const
{
    return reader ? reader->getInt("Logging", "AsyncThreads", 1) : 1;
}

int ConfigManager::getAsyncQueueSize() const
{
    return reader ? reader->getInt("Logging", "AsyncQueueSize", 8192) : 8192;
}

int ConfigManager::getMaxFileSizeMB() const
{
    return reader ? reader->getInt("Logging", "MaxFileSize", 10) : 10; // MB
}

int ConfigManager::getMaxFiles() const
{
    return reader ? reader->getInt("Logging", "MaxFiles", 5) : 5;
}

bool ConfigManager::isDailyRotation() const
{
    return reader ? reader->getBool("Logging", "DailyRotation", false) : false;
}

// Security Configuration
int ConfigManager::getSessionTimeout() const
{
    return reader ? reader->getInt("Security", "SessionTimeout", 3600) : 3600;
}

int ConfigManager::getMaxLoginAttempts() const
{
    return reader ? reader->getInt("Security", "MaxLoginAttempts", 3) : 3;
}

bool ConfigManager::isPasswordEncryptionEnabled() const
{
    return reader ? reader->getBool("Security", "PasswordEncryption", true) : true;
}

int ConfigManager::getPasswordMinLength() const
{
    return reader ? reader->getInt("Security", "PasswordMinLength", 8) : 8;
}

bool ConfigManager::isPasswordRequireUppercase() const
{
    return reader ? reader->getBool("Security", "PasswordRequireUppercase", true) : true;
}

bool ConfigManager::isPasswordRequireLowercase() const
{
    return reader ? reader->getBool("Security", "PasswordRequireLowercase", true) : true;
}

bool ConfigManager::isPasswordRequireNumbers() const
{
    return reader ? reader->getBool("Security", "PasswordRequireNumbers", true) : true;
}

bool ConfigManager::isPasswordRequireSymbols() const
{
    return reader ? reader->getBool("Security", "PasswordRequireSymbols", true) : true;
}

std::string ConfigManager::getJWTSecret() const
{
    return reader ? reader->getString("Security", "JWTSecret", "your_jwt_secret_here") : "your_jwt_secret_here";
}

int ConfigManager::getJWTExpiration() const
{
    return reader ? reader->getInt("Security", "JWTExpiration", 86400) : 86400;
}

// Performance Configuration
int ConfigManager::getThreadPoolSize() const
{
    return reader ? reader->getInt("Performance", "ThreadPoolSize", 4) : 4;
}

int ConfigManager::getConnectionTimeout() const
{
    return reader ? reader->getInt("Performance", "ConnectionTimeout", 30) : 30;
}

bool ConfigManager::isKeepAlive() const
{
    return reader ? reader->getBool("Performance", "KeepAlive", true) : true;
}

int ConfigManager::getKeepAliveTimeout() const
{
    return reader ? reader->getInt("Performance", "KeepAliveTimeout", 60) : 60;
}

int ConfigManager::getReceiveBufferSize() const
{
    return reader ? reader->getInt("Performance", "ReceiveBufferSize", 8192) : 8192;
}

int ConfigManager::getSendBufferSize() const
{
    return reader ? reader->getInt("Performance", "SendBufferSize", 8192) : 8192;
}

int ConfigManager::getRequestTimeout() const
{
    return reader ? reader->getInt("Performance", "RequestTimeout", 30) : 30;
}

// Cache Configuration
bool ConfigManager::isCacheEnabled() const
{
    return reader ? reader->getBool("Cache", "EnableCache", true) : true;
}

int ConfigManager::getCacheSize() const
{
    return reader ? reader->getInt("Cache", "CacheSize", 1000) : 1000;
}

int ConfigManager::getCacheTimeout() const
{
    return reader ? reader->getInt("Cache", "CacheTimeout", 300) : 300;
}

int ConfigManager::getSessionCacheSize() const
{
    return reader ? reader->getInt("Cache", "SessionCacheSize", 500) : 500;
}

int ConfigManager::getSessionCacheTimeout() const
{
    return reader ? reader->getInt("Cache", "SessionCacheTimeout", 1800) : 1800;
}

// Monitoring Configuration
bool ConfigManager::isHealthCheckEnabled() const
{
    return reader ? reader->getBool("Monitoring", "EnableHealthCheck", true) : true;
}

int ConfigManager::getHealthCheckPort() const
{
    return reader ? reader->getInt("Monitoring", "HealthCheckPort", 9090) : 9090;
}

bool ConfigManager::isMetricsEnabled() const
{
    return reader ? reader->getBool("Monitoring", "EnableMetrics", true) : true;
}

std::string ConfigManager::getMetricsEndpoint() const
{
    return reader ? reader->getString("Monitoring", "MetricsEndpoint", "/metrics") : "/metrics";
}

bool ConfigManager::isMonitorCPU() const
{
    return reader ? reader->getBool("Monitoring", "MonitorCPU", true) : true;
}

bool ConfigManager::isMonitorMemory() const
{
    return reader ? reader->getBool("Monitoring", "MonitorMemory", true) : true;
}

bool ConfigManager::isMonitorConnections() const
{
    return reader ? reader->getBool("Monitoring", "MonitorConnections", true) : true;
}

// Features Configuration
bool ConfigManager::isUserRegistrationEnabled() const
{
    return reader ? reader->getBool("Features", "EnableUserRegistration", true) : true;
}

bool ConfigManager::isPasswordResetEnabled() const
{
    return reader ? reader->getBool("Features", "EnablePasswordReset", true) : true;
}

bool ConfigManager::isTwoFactorAuthEnabled() const
{
    return reader ? reader->getBool("Features", "EnableTwoFactorAuth", false) : false;
}

bool ConfigManager::isEmailVerificationEnabled() const
{
    return reader ? reader->getBool("Features", "EnableEmailVerification", true) : true;
}

bool ConfigManager::isRateLimitEnabled() const
{
    return reader ? reader->getBool("Features", "EnableRateLimit", true) : true;
}

int ConfigManager::getRateLimitRequests() const
{
    return reader ? reader->getInt("Features", "RateLimitRequests", 100) : 100;
}

int ConfigManager::getRateLimitWindow() const
{
    return reader ? reader->getInt("Features", "RateLimitWindow", 60) : 60;
}

// Network Configuration
bool ConfigManager::isSSLEnabled() const
{
    return reader ? reader->getBool("Network", "EnableSSL", false) : false;
}

std::string ConfigManager::getSSLCertFile() const
{
    return reader ? reader->getString("Network", "SSLCertFile", "cert.pem") : "cert.pem";
}

std::string ConfigManager::getSSLKeyFile() const
{
    return reader ? reader->getString("Network", "SSLKeyFile", "key.pem") : "key.pem";
}

bool ConfigManager::isCORSEnabled() const
{
    return reader ? reader->getBool("Network", "EnableCORS", true) : true;
}

std::string ConfigManager::getAllowedOrigins() const
{
    return reader ? reader->getString("Network", "AllowedOrigins", "*") : "*";
}

std::string ConfigManager::getAllowedMethods() const
{
    return reader ? reader->getString("Network", "AllowedMethods", "GET,POST,PUT,DELETE") : "GET,POST,PUT,DELETE";
}

std::string ConfigManager::getAllowedHeaders() const
{
    return reader ? reader->getString("Network", "AllowedHeaders", "*") : "*";
}

// Development Configuration
bool ConfigManager::isDebugMode() const
{
    return reader ? reader->getBool("Development", "DebugMode", false) : false;
}

bool ConfigManager::isProfilerEnabled() const
{
    return reader ? reader->getBool("Development", "EnableProfiler", false) : false;
}

bool ConfigManager::isShowDetailedErrors() const
{
    return reader ? reader->getBool("Development", "ShowDetailedErrors", false) : false;
}

// Print all configuration (for debugging)
// MySQL Database Configuration - Account DB
std::string ConfigManager::getAccountDBHost() const
{
    return reader ? reader->getString("Database.Account", "Host", "192.168.91.128") : "192.168.91.128";
}

int ConfigManager::getAccountDBPort() const
{
    return reader ? reader->getInt("Database.Account", "Port", 3306) : 3306;
}

std::string ConfigManager::getAccountDBDatabase() const
{
    return reader ? reader->getString("Database.Account", "Database", "account") : "account";
}

std::string ConfigManager::getAccountDBUsername() const
{
    return reader ? reader->getString("Database.Account", "Username", "root") : "root";
}

std::string ConfigManager::getAccountDBPassword() const
{
    return reader ? reader->getString("Database.Account", "Password", "potato") : "potato";
}

int ConfigManager::getAccountDBMinPoolSize() const
{
    return reader ? reader->getInt("Database.Account", "MinPoolSize", 1) : 1;
}

int ConfigManager::getAccountDBMaxPoolSize() const
{
    return reader ? reader->getInt("Database.Account", "MaxPoolSize", 10) : 10;
}

int ConfigManager::getAccountDBConnectionTimeout() const
{
    return reader ? reader->getInt("Database.Account", "ConnectionTimeout", 30) : 30;
}

int ConfigManager::getAccountDBQueryTimeout() const
{
    return reader ? reader->getInt("Database.Account", "QueryTimeout", 60) : 60;
}

// MySQL Database Configuration - Game DB
std::string ConfigManager::getGameDBHost() const
{
    return reader ? reader->getString("Database.Game", "Host", "192.168.91.128") : "192.168.91.128";
}

int ConfigManager::getGameDBPort() const
{
    return reader ? reader->getInt("Database.Game", "Port", 3306) : 3306;
}

std::string ConfigManager::getGameDBDatabase() const
{
    return reader ? reader->getString("Database.Game", "Database", "game") : "game";
}

std::string ConfigManager::getGameDBUsername() const
{
    return reader ? reader->getString("Database.Game", "Username", "root") : "root";
}

std::string ConfigManager::getGameDBPassword() const
{
    return reader ? reader->getString("Database.Game", "Password", "potato") : "potato";
}

int ConfigManager::getGameDBMinPoolSize() const
{
    return reader ? reader->getInt("Database.Game", "MinPoolSize", 1) : 1;
}

int ConfigManager::getGameDBMaxPoolSize() const
{
    return reader ? reader->getInt("Database.Game", "MaxPoolSize", 10) : 10;
}

int ConfigManager::getGameDBConnectionTimeout() const
{
    return reader ? reader->getInt("Database.Game", "ConnectionTimeout", 30) : 30;
}

int ConfigManager::getGameDBQueryTimeout() const
{
    return reader ? reader->getInt("Database.Game", "QueryTimeout", 60) : 60;
}



void ConfigManager::printConfig() const
{
    std::cout << "=== Configuration ===" << std::endl;
    std::cout << "Config File: " << configFile << std::endl;
    
    if (reader && reader->isValidFile())
    {
        reader->printAll();
    }
    else
    {
        std::cout << "Using default configuration values:" << std::endl;
        std::cout << "Server Port: " << getServerPort() << std::endl;
        std::cout << "Server Host: " << getServerHost() << std::endl;
        std::cout << "Max Connections: " << getMaxConnections() << std::endl;
        std::cout << "Database Host: " << getDatabaseHost() << std::endl;
        std::cout << "Database Port: " << getDatabasePort() << std::endl;
        std::cout << "Database Name: " << getDatabaseName() << std::endl;
        std::cout << "Database User: " << getDatabaseUser() << std::endl;
        std::cout << "Log Level: " << getLogLevel() << std::endl;
        std::cout << "Log File: " << getLogFilePath() << std::endl;
        std::cout << "Session Timeout: " << getSessionTimeout() << std::endl;
        std::cout << "Thread Pool Size: " << getThreadPoolSize() << std::endl;
    }
    std::cout << "===================" << std::endl;
}