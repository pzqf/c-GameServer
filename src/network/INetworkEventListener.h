#pragma once

#include "SocketTypes.h"
#include "AsyncIO.h"

#include <string>
#include <memory>

// 网络事件监听器接口 - 用于解耦网络层和业务层
class INetworkEventListener {
public:
    virtual ~INetworkEventListener() = default;
    
    // 处理客户端连接事件
    virtual void onClientConnected(socket_t clientSocket) = 0;
    
    // 处理客户端断开连接事件
    virtual void onClientDisconnected(socket_t clientSocket) = 0;
    
    // 处理接收到的网络数据
    virtual void onDataReceived(socket_t clientSocket, const std::vector<uint8_t>& data) = 0;
    
    // 处理数据发送完成事件
    virtual void onDataSent(socket_t clientSocket, size_t bytesSent) = 0;
    
    // 处理网络错误事件
    virtual void onNetworkError(socket_t clientSocket, const std::string& errorMessage) = 0;
    
    // 获取监听器名称（用于日志）
    virtual std::string getListenerName() const = 0;
};