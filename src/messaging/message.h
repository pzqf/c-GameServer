#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>
#include <unordered_map>

// 消息类型
enum class MessageType {
    LOGIN = 1,
    REGISTER = 2,
    LOGOUT = 3,
    QUERY_DATA = 4,
    UPDATE_DATA = 5,
    CUSTOM = 100
};

// 消息基类
class Message {
public:
    Message(MessageType type, const std::string& payload, const std::string& clientId = "")
        : type_(type), payload_(payload), clientId_(clientId), id_(generateId()), timestamp_(std::chrono::system_clock::now()) {}

    virtual ~Message() = default;

    MessageType getType() const { return type_; }
    const std::string& getPayload() const { return payload_; }
    const std::string& getClientId() const { return clientId_; }
    size_t getId() const { return id_; }
    std::chrono::system_clock::time_point getTimestamp() const { return timestamp_; }

private:
    MessageType type_;
    std::string payload_;
    std::string clientId_;
    size_t id_;
    std::chrono::system_clock::time_point timestamp_;

    static size_t generateId() {
        static std::atomic<size_t> counter{0};
        return counter.fetch_add(1);
    }
};

// 具体消息类型
class LoginMessage : public Message {
public:
    LoginMessage(const std::string& username, const std::string& password, const std::string& clientId = "")
        : Message(MessageType::LOGIN, "{}", clientId), username_(username), password_(password) {
        // 构造实际的消息体
    }

    const std::string& getUsername() const { return username_; }
    const std::string& getPassword() const { return password_; }

private:
    std::string username_;
    std::string password_;
};

class RegisterMessage : public Message {
public:
    RegisterMessage(const std::string& username, const std::string& password, const std::string& email, const std::string& clientId = "")
        : Message(MessageType::REGISTER, "{}", clientId), username_(username), password_(password), email_(email) {}

    const std::string& getUsername() const { return username_; }
    const std::string& getPassword() const { return password_; }
    const std::string& getEmail() const { return email_; }

private:
    std::string username_;
    std::string password_;
    std::string email_;
};

// 使用unique_ptr以便管理动态分配的消息
using MessagePtr = std::unique_ptr<Message>;
using LoginMessagePtr = std::unique_ptr<LoginMessage>;
using RegisterMessagePtr = std::unique_ptr<RegisterMessage>;

// 消息处理函数类型定义
using MessageHandler = std::function<void(const Message&)>;

// 获取消息类型的字符串名称
inline const char* getMessageTypeName(MessageType type) {
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

// 线程安全的消息队列
class MessageQueue {
public:
    MessageQueue() = default;
    ~MessageQueue() = default;

    // 禁止拷贝和移动
    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;
    MessageQueue(MessageQueue&&) = delete;
    MessageQueue& operator=(MessageQueue&&) = delete;

    // 添加消息到队列（生产者调用）
    void push(MessagePtr message);

    // 从队列中获取消息（消费者调用）
    // 如果队列为空且wait为true，则阻塞等待
    // 如果wait为false且队列为空，则返回nullptr
    MessagePtr pop(bool wait = true);

    // 检查队列是否为空
    bool empty() const;

    // 获取队列大小
    size_t size() const;

    // 关闭队列，不再接受新消息，并通知等待线程
    void shutdown();

    // 检查队列是否已关闭
    bool is_shutdown() const;

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<MessagePtr> queue_;
    std::atomic<bool> shutdown_{false};
};