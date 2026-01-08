#pragma once
#include <string>
#include "INIReader.h"

class ConfigManager
{
private:
    INIReader* reader;
    std::string configFile;

public:
    ConfigManager(const std::string& configFile = "config.ini");
    ~ConfigManager();

    void loadConfig();

    // Server Configuration
    int getServerPort() const;
    std::string getServerHost() const;
    std::string getServerName() const;
    int getMaxConnections() const;

    // Database Configuration (MySQL format)
    std::string getDatabaseHost() const;
    int getDatabasePort() const;
    std::string getDatabaseName() const;
    std::string getDatabaseUser() const;
    std::string getDatabasePassword() const;
    int getDatabaseMaxConnections() const;
    int getDatabaseConnectionTimeout() const;
    int getDatabaseMaxRetries() const;
    int getDatabaseRetryDelay() const;

    // MySQL Database Configuration for Account database
    std::string getAccountDBHost() const;
    int getAccountDBPort() const;
    std::string getAccountDBDatabase() const;
    std::string getAccountDBUsername() const;
    std::string getAccountDBPassword() const;
    int getAccountDBMinPoolSize() const;
    int getAccountDBMaxPoolSize() const;
    int getAccountDBConnectionTimeout() const;
    int getAccountDBQueryTimeout() const;

    // MySQL Database Configuration for Game database
    std::string getGameDBHost() const;
    int getGameDBPort() const;
    std::string getGameDBDatabase() const;
    std::string getGameDBUsername() const;
    std::string getGameDBPassword() const;
    int getGameDBMinPoolSize() const;
    int getGameDBMaxPoolSize() const;
    int getGameDBConnectionTimeout() const;
    int getGameDBQueryTimeout() const;



    // Logging Configuration (Unified)
    std::string getLogLevel() const;
    std::string getLogFilePath() const;
    bool isConsoleLogging() const;
    bool isFileLogging() const;
    std::string getLogPattern() const;
    bool isAsyncLogging() const;
    int getAsyncThreadCount() const;
    int getAsyncQueueSize() const;
    int getMaxFileSizeMB() const;
    int getMaxFiles() const;
    bool isDailyRotation() const;

    // Security Configuration
    int getSessionTimeout() const;
    int getMaxLoginAttempts() const;
    bool isPasswordEncryptionEnabled() const;
    int getPasswordMinLength() const;
    bool isPasswordRequireUppercase() const;
    bool isPasswordRequireLowercase() const;
    bool isPasswordRequireNumbers() const;
    bool isPasswordRequireSymbols() const;
    std::string getJWTSecret() const;
    int getJWTExpiration() const;

    // Performance Configuration
    int getThreadPoolSize() const;
    int getConnectionTimeout() const;
    bool isKeepAlive() const;
    int getKeepAliveTimeout() const;
    int getReceiveBufferSize() const;
    int getSendBufferSize() const;
    int getRequestTimeout() const;

    // Cache Configuration
    bool isCacheEnabled() const;
    int getCacheSize() const;
    int getCacheTimeout() const;
    int getSessionCacheSize() const;
    int getSessionCacheTimeout() const;

    // Monitoring Configuration
    bool isHealthCheckEnabled() const;
    int getHealthCheckPort() const;
    bool isMetricsEnabled() const;
    std::string getMetricsEndpoint() const;
    bool isMonitorCPU() const;
    bool isMonitorMemory() const;
    bool isMonitorConnections() const;

    // Features Configuration
    bool isUserRegistrationEnabled() const;
    bool isPasswordResetEnabled() const;
    bool isTwoFactorAuthEnabled() const;
    bool isEmailVerificationEnabled() const;
    bool isRateLimitEnabled() const;
    int getRateLimitRequests() const;
    int getRateLimitWindow() const;

    // Network Configuration
    bool isSSLEnabled() const;
    std::string getSSLCertFile() const;
    std::string getSSLKeyFile() const;
    bool isCORSEnabled() const;
    std::string getAllowedOrigins() const;
    std::string getAllowedMethods() const;
    std::string getAllowedHeaders() const;

    // Development Configuration
    bool isDebugMode() const;
    bool isProfilerEnabled() const;
    bool isShowDetailedErrors() const;

    // Print all configuration (for debugging)
    void printConfig() const;
};