# CMakeList.txt: AccountSvr 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.10)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("GameServer")

# 添加spdlog库
add_subdirectory(common/spdlog-1.x)

# 包含spdlog头文件目录
include_directories(${CMAKE_SOURCE_DIR}/common/spdlog-1.x/include)

# 将源代码添加到此项目的可执行文件。
add_executable (GameServer 
    "src/GameServer.cpp"
    "include/INIReader.h"
    "src/config/ConfigManager.h" 
    "src/config/ConfigManager.cpp" 

    "src/network/NetworkServer.h" 
    "src/network/NetworkServer.cpp"

    "src/database/DatabaseManager.h"
    "src/database/DatabaseManager.cpp"
    "src/database/DatabaseRepository.h"
    "src/database/AccountDB.h"
    "src/database/AccountDB.cpp"
    "src/database/AccountRepository.h"
    "src/database/CCURepository.h"

    "src/database/MySQLConnection.h"
    "src/database/MySQLConnection.cpp"
    "src/database/MySQLPool.h"
    "src/database/MySQLPool.cpp"
    
    "src/messaging/message.h"
    "src/messaging/message.cpp"
    "src/messaging/result.h"
    "src/messaging/message_header.h"
    "src/handler/message_handler.h"
    "src/handler/message_handler.cpp"
    "src/handler/MainLoopHandler.h"
    "src/handler/MainLoopHandler.cpp"
    "src/network/NetworkServer_fwd.h"
    "src/main/MainLoop.h"
    "src/main/MainLoop.cpp"
    
    "src/logging/Log.h"
    "src/logging/Log.cpp"
    
    "src/network/AsyncIO.h"
    "src/network/AsyncIOManager.cpp"
    "src/network/WindowsIOCP.h"
    "src/network/WindowsIOCP.cpp"
    "src/network/LinuxEpoll.h"
    "src/network/LinuxEpoll.cpp"
)
add_executable (ClientTest "tests/ClientTest.cpp")
add_executable (MySQLTest 
    "tests/MySQLTest.cpp"
    "src/database/DatabaseManager.cpp"
    "src/database/MySQLConnection.cpp"
    "src/database/MySQLPool.cpp"
    "src/config/ConfigManager.cpp"
    "src/logging/Log.cpp"
)

# 添加头文件搜索路径
target_include_directories(GameServer PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(GameServer PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_include_directories(GameServer PRIVATE "${CMAKE_SOURCE_DIR}/src/logging")
target_include_directories(GameServer PRIVATE "${CMAKE_SOURCE_DIR}/src/network")
target_include_directories(GameServer PRIVATE "${CMAKE_SOURCE_DIR}/src/config")
target_include_directories(GameServer PRIVATE "${CMAKE_SOURCE_DIR}/src/database")
target_include_directories(GameServer PRIVATE "${CMAKE_SOURCE_DIR}/src/messaging")
target_include_directories(GameServer PRIVATE "${CMAKE_SOURCE_DIR}/src/main")
target_include_directories(GameServer PRIVATE "${CMAKE_SOURCE_DIR}/src/handler")
target_include_directories(MySQLTest PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(MySQLTest PRIVATE "${CMAKE_SOURCE_DIR}/src/network")
target_include_directories(MySQLTest PRIVATE "${CMAKE_SOURCE_DIR}/src/logging")
target_include_directories(MySQLTest PRIVATE "${CMAKE_SOURCE_DIR}/src/config")
target_include_directories(MySQLTest PRIVATE "${CMAKE_SOURCE_DIR}/src/database")
target_include_directories(MySQLTest PRIVATE "${CMAKE_SOURCE_DIR}/src/messaging")
target_include_directories(MySQLTest PRIVATE "${CMAKE_SOURCE_DIR}/src/main")
target_include_directories(MySQLTest PRIVATE "${CMAKE_SOURCE_DIR}/common/mysql-connector-c++-9.4.0-winx64/include")
target_include_directories(GameServer PRIVATE "${CMAKE_SOURCE_DIR}/common/mysql-connector-c++-9.4.0-winx64/include")

# 链接spdlog库
target_link_libraries(GameServer spdlog)
if (WIN32)
    target_link_libraries(GameServer ws2_32)
    target_link_libraries(GameServer "${CMAKE_SOURCE_DIR}/common/mysql-connector-c++-9.4.0-winx64/lib64/vs14/mysqlcppconn.lib")
endif()
target_link_libraries(ClientTest spdlog)
if (WIN32)
    target_link_libraries(ClientTest ws2_32)
endif()
target_link_libraries(MySQLTest spdlog)
if (WIN32)
    target_link_libraries(MySQLTest ws2_32)
    target_link_libraries(MySQLTest "${CMAKE_SOURCE_DIR}/common/mysql-connector-c++-9.4.0-winx64/lib64/vs14/mysqlcppconn.lib")
endif()

# 设置C++标准为C++17以支持内联变量
set_property(TARGET GameServer PROPERTY CXX_STANDARD 17)
set_property(TARGET ClientTest PROPERTY CXX_STANDARD 17)
set_property(TARGET MySQLTest PROPERTY CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

# TODO: 如有需要，请添加测试并安装目标。