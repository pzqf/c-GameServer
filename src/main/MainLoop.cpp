#include "MainLoop.h"
#include "database/DatabaseManager.h"
#include "Log.h"
#include "network/NetworkServer.h"
#include "messaging/result.h"
#include "../messaging/message.h"
#include <stdexcept>
#include <chrono>

MainLoop::MainLoop() : running_(false), accountDB_(nullptr), networkServer_(nullptr) {
    try {
        accountDB_ = &AccountDB::getInstance();
        LOG_INFO("MainLoop initialized");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize MainLoop: {}", e.what());
        throw;
    }
}

MainLoop::~MainLoop() {
    stop();
}

void MainLoop::start() {
    if (running_) {
        LOG_WARN("MainLoop is already running");
        return;
    }
    
    LOG_INFO("Starting MainLoop");
    running_ = true;
    
    loopThread_ = std::thread(&MainLoop::runLoop, this);
}

void MainLoop::stop() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Stopping MainLoop");
    running_ = false;
    
    if (loopThread_.joinable()) {
        loopThread_.join();
    }
    
    LOG_INFO("MainLoop stopped");
}

bool MainLoop::isRunning() const {
    return running_;
}

MessagePtr MainLoop::getNextMessage() {
    return messageQueue_.pop(false);
}

void MainLoop::addMessage(MessagePtr message) {
    if (message) {
        messageQueue_.push(std::move(message));
    }
}

void MainLoop::runLoop() {
    LOG_INFO("MainLoop thread started");
    
    while (running_) {
        try {
            auto message = getNextMessage();
            
            if (message) {
                messageHandler_.handleMessage(*message);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in MainLoop: {}", e.what());
        }
    }
    
    LOG_INFO("MainLoop thread ended");
}
