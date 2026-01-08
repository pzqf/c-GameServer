#include "message_handler.h"
#include "../database/AccountDB.h"
#include "../logging/Log.h"
#include <sstream>
#include <iomanip>

std::unique_ptr<Message> convertNetworkMessageToMessage(const NetworkMessage& networkMessage, uint64_t clientId) {
    const auto& header = networkMessage.getHeader();
    const auto& body = networkMessage.getBody();
    
    MessageType type;
    uint32_t messageId = header.messageId;
    
    switch (messageId) {
        case MessageIds::LOGIN:
            type = MessageType::LOGIN;
            break;
        case MessageIds::REGISTER:
            type = MessageType::REGISTER;
            break;
        case MessageIds::LOGOUT:
            type = MessageType::LOGOUT;
            break;
        case MessageIds::QUERY_DATA:
            type = MessageType::QUERY_DATA;
            break;
        case MessageIds::UPDATE_DATA:
            type = MessageType::UPDATE_DATA;
            break;
        default:
            type = MessageType::CUSTOM;
            break;
    }
    
    switch (type) {
        case MessageType::LOGIN: {
            std::string username, password;
            std::string bodyData(reinterpret_cast<const char*>(body.getData()), body.getSize());
            size_t pos = bodyData.find('|');
            if (pos != std::string::npos) {
                username = bodyData.substr(0, pos);
                password = bodyData.substr(pos + 1);
            }
            return std::make_unique<LoginMessage>(username, password, std::to_string(clientId));
        }
        case MessageType::REGISTER: {
            std::string username, password, email;
            std::string bodyData(reinterpret_cast<const char*>(body.getData()), body.getSize());
            size_t pos1 = bodyData.find('|');
            size_t pos2 = bodyData.find('|', pos1 + 1);
            if (pos1 != std::string::npos && pos2 != std::string::npos) {
                username = bodyData.substr(0, pos1);
                password = bodyData.substr(pos1 + 1, pos2 - pos1 - 1);
                email = bodyData.substr(pos2 + 1);
            }
            return std::make_unique<RegisterMessage>(username, password, email, std::to_string(clientId));
        }
        default: {
            std::string payload(reinterpret_cast<const char*>(body.getData()), body.getSize());
            return std::make_unique<Message>(type, payload, std::to_string(clientId));
        }
    }
}

std::unique_ptr<NetworkMessage> MessageParser::parseMessage(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(MessageHeader)) {
        return nullptr;
    }
    
    auto message = std::make_unique<NetworkMessage>();
    if (message->deserialize(data.data(), data.size())) {
        return message;
    }
    
    return nullptr;
}

bool MessageParser::isCompleteMessage(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(MessageHeader)) {
        return false;
    }
    
    MessageHeader header;
    if (!header.deserialize(data.data(), data.size())) {
        return false;
    }
    
    size_t expectedSize = sizeof(MessageHeader) + header.dataLength;
    return data.size() >= expectedSize;
}

NetworkMessage MessageParser::createLoginMessage(const std::string& username, const std::string& password) {
    auto body = MessageUtils::packLoginData(username, password);
    return NetworkMessage(MessageIds::LOGIN, body);
}

NetworkMessage MessageParser::createRegisterMessage(const std::string& username, const std::string& password, const std::string& email) {
    auto body = MessageUtils::packRegisterData(username, password, email);
    return NetworkMessage(MessageIds::REGISTER, body);
}

NetworkMessage MessageParser::createResponseMessage(uint32_t originalMessageId, bool success, const std::string& message) {
    if (success) {
        return MessageUtils::createSuccessResponse(originalMessageId, message);
    } else {
        return MessageUtils::createErrorResponse(originalMessageId, message);
    }
}

NetworkMessage MessageUtils::createSuccessResponse(uint32_t originalMessageId, const std::string& message) {
    auto body = packResponseData(message);
    return NetworkMessage(MessageIds::SUCCESS_RESPONSE, body);
}

NetworkMessage MessageUtils::createErrorResponse(uint32_t originalMessageId, const std::string& errorMessage) {
    auto body = packResponseData(errorMessage);
    return NetworkMessage(MessageIds::ERROR_RESPONSE, body);
}

bool MessageUtils::parseLoginData(const MessageBody& body, std::string& username, std::string& password) {
    try {
        std::string data = body.toString();
        
        size_t separator = data.find('|');
        if (separator == std::string::npos) {
            return false;
        }
        
        username = data.substr(0, separator);
        password = data.substr(separator + 1);
        
        return !username.empty() && !password.empty();
    }
    catch (...) {
        return false;
    }
}

bool MessageUtils::parseRegisterData(const MessageBody& body, std::string& username, std::string& password, std::string& email) {
    try {
        std::string data = body.toString();
        
        size_t firstSeparator = data.find('|');
        if (firstSeparator == std::string::npos) {
            return false;
        }
        
        size_t secondSeparator = data.find('|', firstSeparator + 1);
        if (secondSeparator == std::string::npos) {
            return false;
        }
        
        username = data.substr(0, firstSeparator);
        password = data.substr(firstSeparator + 1, secondSeparator - firstSeparator - 1);
        email = data.substr(secondSeparator + 1);
        
        return !username.empty() && !password.empty() && !email.empty();
    }
    catch (...) {
        return false;
    }
}

MessageBody MessageUtils::packLoginData(const std::string& username, const std::string& password) {
    std::string data = username + "|" + password;
    return MessageBody(data);
}

MessageBody MessageUtils::packRegisterData(const std::string& username, const std::string& password, const std::string& email) {
    std::string data = username + "|" + password + "|" + email;
    return MessageBody(data);
}

MessageBody MessageUtils::packResponseData(const std::string& message) {
    return MessageBody(message);
}
