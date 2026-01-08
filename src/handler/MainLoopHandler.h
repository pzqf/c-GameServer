#pragma once
#include <mutex>
#include <unordered_map>
#include <functional>
#include <string>
#include "../messaging/message.h"
#include "../logging/Log.h"

using MessageHandler = std::function<void(const Message&)>;

class MainLoopHandler {
public:
    MainLoopHandler() = default;
    ~MainLoopHandler() = default;

    MainLoopHandler(const MainLoopHandler&) = delete;
    MainLoopHandler& operator=(const MainLoopHandler&) = delete;
    MainLoopHandler(MainLoopHandler&&) = delete;
    MainLoopHandler& operator=(MainLoopHandler&&) = delete;

    bool registerHandler(MessageType type, MessageHandler handler);
    bool unregisterHandler(MessageType type);
    bool hasHandler(MessageType type) const;
    void handleMessage(const Message& message);
    void clear();

private:
    static std::string getMessageTypeName(MessageType type) {
        switch (type) {
            case MessageType::LOGIN: return "LOGIN";
            case MessageType::REGISTER: return "REGISTER";
            case MessageType::LOGOUT: return "LOGOUT";
            case MessageType::QUERY_DATA: return "QUERY_DATA";
            case MessageType::UPDATE_DATA: return "UPDATE_DATA";
            case MessageType::CUSTOM: return "CUSTOM";
            default: return "UNKNOWN";
        }
    }

    std::unordered_map<MessageType, MessageHandler> handlers_;
    mutable std::mutex handlersMutex_;
};
