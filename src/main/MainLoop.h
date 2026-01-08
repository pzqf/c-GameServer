#pragma once
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <mutex>
#include "database/AccountDB.h"
#include "Log.h"
#include "../handler/MainLoopHandler.h"

class NetworkServer;

class MainLoop {
public:
    MainLoop();
    ~MainLoop();

    void start();
    void stop();
    bool isRunning() const;

    MessagePtr getNextMessage();
    void addMessage(MessagePtr message);

    void setNetworkServer(NetworkServer* networkServer) {
        networkServer_ = networkServer;
    }

    MainLoopHandler& getHandler() {
        return messageHandler_;
    }

private:
    void runLoop();

private:
    std::atomic<bool> running_;
    std::thread loopThread_;
    MessageQueue messageQueue_;
    std::mutex messageQueueMutex_;
    AccountDB* accountDB_;
    NetworkServer* networkServer_;
    MainLoopHandler messageHandler_;
};
