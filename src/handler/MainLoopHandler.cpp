#include "MainLoopHandler.h"

bool MainLoopHandler::registerHandler(MessageType type, MessageHandler handler) {
    if (!handler) {
        LOG_ERROR("Attempted to register null handler for message type: {}", getMessageTypeName(type));
        return false;
    }

    std::lock_guard<std::mutex> lock(handlersMutex_);

    auto it = handlers_.find(type);
    if (it != handlers_.end()) {
        LOG_WARN("Handler already registered for message type: {}, overwriting", getMessageTypeName(type));
    }

    handlers_[type] = std::move(handler);
    LOG_INFO("Registered handler for message type: {}", getMessageTypeName(type));
    return true;
}

bool MainLoopHandler::unregisterHandler(MessageType type) {
    std::lock_guard<std::mutex> lock(handlersMutex_);

    size_t erased = handlers_.erase(type);
    if (erased > 0) {
        LOG_INFO("Unregistered handler for message type: {}", getMessageTypeName(type));
        return true;
    }

    LOG_WARN("No handler found to unregister for message type: {}", getMessageTypeName(type));
    return false;
}

bool MainLoopHandler::hasHandler(MessageType type) const {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    return handlers_.find(type) != handlers_.end();
}

void MainLoopHandler::handleMessage(const Message& message) {
    std::lock_guard<std::mutex> lock(handlersMutex_);

    auto it = handlers_.find(message.getType());

    if (it != handlers_.end() && it->second) {
        LOG_DEBUG("Dispatching message type: {} to handler", getMessageTypeName(message.getType()));
        it->second(message);
    } else {
        LOG_WARN("No handler registered for message type: {}", getMessageTypeName(message.getType()));
    }
}

void MainLoopHandler::clear() {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    handlers_.clear();
}
