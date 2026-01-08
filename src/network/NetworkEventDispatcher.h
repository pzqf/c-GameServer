#pragma once

#include "INetworkEventListener.h"
#include "SocketTypes.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

// 网络事件分发器 - 负责管理和分发网络事件
class NetworkEventDispatcher {
private:
    std::vector<std::weak_ptr<INetworkEventListener>> listeners_;
    mutable std::mutex listenersMutex_;
    
public:
    NetworkEventDispatcher() = default;
    ~NetworkEventDispatcher() = default;
    
    // 添加事件监听器
    void addListener(std::shared_ptr<INetworkEventListener> listener) {
        std::lock_guard<std::mutex> lock(listenersMutex_);
        listeners_.push_back(listener);
    }
    
    // 移除事件监听器
    void removeListener(std::shared_ptr<INetworkEventListener> listener) {
        std::lock_guard<std::mutex> lock(listenersMutex_);
        listeners_.erase(
            std::remove_if(listeners_.begin(), listeners_.end(),
                [&listener](const std::weak_ptr<INetworkEventListener>& weak) {
                    if (auto ptr = weak.lock()) {
                        return ptr == listener;
                    }
                    return true; // 移除已失效的weak_ptr
                }),
            listeners_.end()
        );
    }
    
    // 分发客户端连接事件
    void notifyClientConnected(socket_t clientSocket) {
        std::lock_guard<std::mutex> lock(listenersMutex_);
        for (auto& weak : listeners_) {
            if (auto listener = weak.lock()) {
                try {
                    listener->onClientConnected(clientSocket);
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in onClientConnected listener: {}", e.what());
                }
            }
        }
    }
    
    // 分发客户端断开连接事件
    void notifyClientDisconnected(socket_t clientSocket) {
        std::lock_guard<std::mutex> lock(listenersMutex_);
        for (auto& weak : listeners_) {
            if (auto listener = weak.lock()) {
                try {
                    listener->onClientDisconnected(clientSocket);
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in onClientDisconnected listener: {}", e.what());
                }
            }
        }
    }
    
    // 分发数据接收事件
    void notifyDataReceived(socket_t clientSocket, const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(listenersMutex_);
        for (auto& weak : listeners_) {
            if (auto listener = weak.lock()) {
                try {
                    listener->onDataReceived(clientSocket, data);
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in onDataReceived listener: {}", e.what());
                }
            }
        }
    }
    
    // 分发数据发送事件
    void notifyDataSent(socket_t clientSocket, size_t bytesSent) {
        std::lock_guard<std::mutex> lock(listenersMutex_);
        for (auto& weak : listeners_) {
            if (auto listener = weak.lock()) {
                try {
                    listener->onDataSent(clientSocket, bytesSent);
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in onDataSent listener: {}", e.what());
                }
            }
        }
    }
    
    // 分发网络错误事件
    void notifyNetworkError(socket_t clientSocket, const std::string& errorMessage) {
        std::lock_guard<std::mutex> lock(listenersMutex_);
        for (auto& weak : listeners_) {
            if (auto listener = weak.lock()) {
                try {
                    listener->onNetworkError(clientSocket, errorMessage);
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in onNetworkError listener: {}", e.what());
                }
            }
        }
    }
    
    // 清理失效的监听器
    void cleanupExpiredListeners() {
        std::lock_guard<std::mutex> lock(listenersMutex_);
        listeners_.erase(
            std::remove_if(listeners_.begin(), listeners_.end(),
                [](const std::weak_ptr<INetworkEventListener>& weak) {
                    return weak.expired();
                }),
            listeners_.end()
        );
    }
};