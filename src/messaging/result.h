#pragma once

#include <string>
#include <memory>
#include <future>
#include <vector>
#include <map>
#include <mutex>

enum class ResponseType {
    SUCCESS,
    SERVICE_ERROR,
    NOT_FOUND,
    INVALID_FORMAT,
    DATABASE_ERROR
};

class OperationResult {
public:
    OperationResult(size_t id, ResponseType type, const std::string& message = "")
        : id_(id), type_(type), message_(message), completed_(false) {}

    OperationResult(OperationResult&& other) noexcept
        : id_(other.id_), type_(other.type_), message_(std::move(other.message_)), 
          completed_(other.completed_) {}

    OperationResult& operator=(OperationResult&& other) noexcept {
        if (this != &other) {
            id_ = other.id_;
            type_ = other.type_;
            message_ = std::move(other.message_);
            completed_ = other.completed_;
        }
        return *this;
    }

    OperationResult(const OperationResult&) = delete;
    OperationResult& operator=(const OperationResult&) = delete;

    size_t getId() const { return id_; }
    ResponseType getType() const { return type_; }
    const std::string& getMessage() const { return message_; }

    void setData(const std::string& data) {
        data_ = data;
        completed_ = true;
    }

    bool isCompleted() const { return completed_; }
    const std::string& getData() const { return data_; }

private:
    size_t id_;
    ResponseType type_;
    std::string message_;
    std::string data_;
    bool completed_;
};

using OperationResultPtr = std::unique_ptr<OperationResult>;