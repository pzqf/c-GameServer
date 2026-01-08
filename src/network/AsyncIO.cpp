#include "AsyncIO.h"
#include "LinuxEpoll.h"
#include "WindowsIOCP.h"
#include <memory>

std::unique_ptr<AsyncIO> AsyncIOFactory::createAsyncIO() {
#ifdef __linux__
    return std::make_unique<LinuxEpoll>();
#elif defined(_WIN32)
    return std::make_unique<WindowsIOCP>();
#else
    // 对于其他平台，可以使用select模型作为后备
    return nullptr;
#endif
}

