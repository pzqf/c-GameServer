#include "message.h"
#include <stdexcept>

// MessageQueue implementation
void MessageQueue::push(MessagePtr message) {
    if (!message) {
        throw std::invalid_argument("Cannot push null message to queue");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (shutdown_) {
        throw std::runtime_error("Cannot push message to shutdown queue");
    }
    
    queue_.push(std::move(message));
    condition_.notify_one();
}

MessagePtr MessageQueue::pop(bool wait) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (wait) {
        // 等待条件变量，直到队列非空或关闭
        condition_.wait(lock, [this] {
            return shutdown_ || !queue_.empty();
        });
        
        // 如果队列已关闭且为空，返回nullptr
        if (shutdown_ && queue_.empty()) {
            return nullptr;
        }
    } else {
        // 如果队列为空且不等待，立即返回
        if (queue_.empty()) {
            return nullptr;
        }
    }
    
    // 获取并移动消息
    MessagePtr message = std::move(const_cast<MessagePtr&>(queue_.front()));
    queue_.pop();
    
    return message;
}

bool MessageQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t MessageQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void MessageQueue::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown_.store(true);
    condition_.notify_all();
}

bool MessageQueue::is_shutdown() const {
    return shutdown_.load();
}