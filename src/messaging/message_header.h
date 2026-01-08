#pragma once

#include <cstdint>
#include <string>
#include <vector>

// 消息头结构体 - 固定长度，包含消息ID和数据长度
struct MessageHeader {
    uint32_t messageId;     // 消息ID (4字节)
    uint32_t dataLength;    // 消息体数据长度 (4字节)
    
    MessageHeader() : messageId(0), dataLength(0) {}
    
    MessageHeader(uint32_t id, uint32_t length) 
        : messageId(id), dataLength(length) {}
    
    // 序列化为字节流
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data(8);
        data[0] = (messageId >> 24) & 0xFF;
        data[1] = (messageId >> 16) & 0xFF;
        data[2] = (messageId >> 8) & 0xFF;
        data[3] = messageId & 0xFF;
        data[4] = (dataLength >> 24) & 0xFF;
        data[5] = (dataLength >> 16) & 0xFF;
        data[6] = (dataLength >> 8) & 0xFF;
        data[7] = dataLength & 0xFF;
        return data;
    }
    
    // 从字节流反序列化
    bool deserialize(const uint8_t* data, size_t size) {
        if (size < 8) return false;
        
        messageId = (static_cast<uint32_t>(data[0]) << 24) |
                   (static_cast<uint32_t>(data[1]) << 16) |
                   (static_cast<uint32_t>(data[2]) << 8) |
                   static_cast<uint32_t>(data[3]);
        
        dataLength = (static_cast<uint32_t>(data[4]) << 24) |
                    (static_cast<uint32_t>(data[5]) << 16) |
                    (static_cast<uint32_t>(data[6]) << 8) |
                    static_cast<uint32_t>(data[7]);
        
        return true;
    }
};

// 消息体类 - 存储二进制数据
class MessageBody {
private:
    std::vector<uint8_t> data_;
    
public:
    MessageBody() = default;
    
    explicit MessageBody(const std::vector<uint8_t>& data) : data_(data) {}
    
    explicit MessageBody(const std::string& str) {
        data_.assign(str.begin(), str.end());
    }
    
    // 获取二进制数据
    const uint8_t* getData() const { return data_.data(); }
    size_t getSize() const { return data_.size(); }
    
    // 设置二进制数据
    void setData(const uint8_t* data, size_t size) {
        data_.assign(data, data + size);
    }
    
    void setData(const std::vector<uint8_t>& data) {
        data_ = data;
    }
    
    // 字符串转换
    std::string toString() const {
        return std::string(data_.begin(), data_.end());
    }
    
    void fromString(const std::string& str) {
        data_.assign(str.begin(), str.end());
    }
    
    // 清空数据
    void clear() {
        data_.clear();
    }
    
    // 检查是否为空
    bool empty() const {
        return data_.empty();
    }
};

// 网络消息类 - 包含消息头和消息体
class NetworkMessage {
private:
    MessageHeader header_;
    MessageBody body_;
    
public:
    NetworkMessage() = default;
    
    NetworkMessage(uint32_t messageId, const MessageBody& body)
        : header_(messageId, body.getSize()), body_(body) {}
    
    NetworkMessage(uint32_t messageId, const std::vector<uint8_t>& data)
        : header_(messageId, data.size()), body_(data) {}
    
    // 获取消息头
    const MessageHeader& getHeader() const { return header_; }
    MessageHeader& getHeader() { return header_; }
    
    // 获取消息体
    const MessageBody& getBody() const { return body_; }
    MessageBody& getBody() { return body_; }
    
    // 获取完整消息大小 (头 + 体)
    size_t getTotalSize() const {
        return sizeof(MessageHeader) + header_.dataLength;
    }
    
    // 序列化为字节流
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> result;
        
        // 添加消息头
        auto headerData = header_.serialize();
        result.insert(result.end(), headerData.begin(), headerData.end());
        
        // 添加消息体
        if (!body_.empty()) {
            result.insert(result.end(), body_.getData(), body_.getData() + body_.getSize());
        }
        
        return result;
    }
    
    // 从字节流反序列化
    bool deserialize(const uint8_t* data, size_t size) {
        // 首先解析消息头
        if (!header_.deserialize(data, size)) {
            return false;
        }
        
        // 检查数据大小是否足够
        size_t expectedSize = sizeof(MessageHeader) + header_.dataLength;
        if (size < expectedSize) {
            return false;
        }
        
        // 提取消息体
        if (header_.dataLength > 0) {
            body_.setData(data + sizeof(MessageHeader), header_.dataLength);
        } else {
            body_.clear();
        }
        
        return true;
    }
};

// 预定义的消息ID常量
namespace MessageIds {
    constexpr uint32_t LOGIN = 1001;
    constexpr uint32_t REGISTER = 1002;
    constexpr uint32_t LOGOUT = 1003;
    constexpr uint32_t QUERY_DATA = 2001;
    constexpr uint32_t UPDATE_DATA = 2002;
    constexpr uint32_t HEARTBEAT = 3001;
    constexpr uint32_t ERROR_RESPONSE = 9001;
    constexpr uint32_t SUCCESS_RESPONSE = 9002;
}