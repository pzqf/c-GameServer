#pragma once

#include "../messaging/message_header.h"
#include "../messaging/message.h"
#include <memory>
#include <functional>
#include <string>
#include <cstdint>

std::unique_ptr<Message> convertNetworkMessageToMessage(const NetworkMessage& networkMessage, uint64_t clientId);

class MessageParser {
public:
    static std::unique_ptr<NetworkMessage> parseMessage(const std::vector<uint8_t>& data);
    
    static bool isCompleteMessage(const std::vector<uint8_t>& data);
    
    static NetworkMessage createLoginMessage(const std::string& username, const std::string& password);
    
    static NetworkMessage createRegisterMessage(const std::string& username, const std::string& password, const std::string& email);
    
    static NetworkMessage createResponseMessage(uint32_t originalMessageId, bool success, const std::string& message);
    
private:
    MessageParser() = default;
};

class MessageUtils {
public:
    static NetworkMessage createSuccessResponse(uint32_t originalMessageId, const std::string& message = "");
    
    static NetworkMessage createErrorResponse(uint32_t originalMessageId, const std::string& errorMessage);
    
    static bool parseLoginData(const MessageBody& body, std::string& username, std::string& password);
    
    static bool parseRegisterData(const MessageBody& body, std::string& username, std::string& password, std::string& email);
    
    static MessageBody packLoginData(const std::string& username, const std::string& password);
    
    static MessageBody packRegisterData(const std::string& username, const std::string& password, const std::string& email);
    
    static MessageBody packResponseData(const std::string& message);
    
private:
    MessageUtils() = default;
};
