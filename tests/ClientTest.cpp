#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "spdlog/spdlog.h"

// 使用我们的跨平台抽象层
#include "../src/network/SocketTypes.h"

#ifdef _WIN32
    #include <ws2tcpip.h>
    #include <windows.h>
#else
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

class ClientTest
{
private:
    socket_t clientSocket;
    std::string serverHost;
    int serverPort;
    bool connected;

public:
    ClientTest(const std::string& host = "localhost", int port = 8080)
        : clientSocket(INVALID_SOCKET_VALUE), serverHost(host), serverPort(port), connected(false)
    {
#ifdef _WIN32
        // 初始化Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            spdlog::error("WSAStartup failed");
        }
#endif
    }

    ~ClientTest()
    {
        disconnect();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool connectToServer()
    {
        // 创建套接字
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET_VALUE)
        {
            spdlog::error("Failed to create socket: {}", GET_LAST_ERROR());
            return false;
        }

        // 设置服务器地址
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);

        // 解析服务器IP地址
        if (inet_pton(AF_INET, serverHost.c_str(), &serverAddr.sin_addr) <= 0)
        {
            // 如果不是IP地址，尝试解析主机名
            struct hostent* host = gethostbyname(serverHost.c_str());
            if (host == nullptr)
            {
                spdlog::error("Failed to resolve hostname: {}", serverHost);
                return false;
            }
            serverAddr.sin_addr = *(struct in_addr*)host->h_addr;
        }

        // 连接到服务器
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            spdlog::error("Connection failed: {}", WSAGetLastError());
            return false;
        }

        connected = true;
        spdlog::info("Connected to server at {}:{}", serverHost, serverPort);
        return true;
    }

    void disconnect()
    {
        if (connected && clientSocket != INVALID_SOCKET)
        {
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
            connected = false;
            spdlog::info("Disconnected from server");
        }
    }

    bool sendMessage(const std::string& message)
    {
        if (!connected)
        {
            spdlog::warn("Not connected to server");
            return false;
        }

        std::string fullMessage = message + "\n";
        int bytesSent = send(clientSocket, fullMessage.c_str(), fullMessage.length(), 0);
        
        if (bytesSent == SOCKET_ERROR)
        {
            spdlog::error("Send failed: {}", WSAGetLastError());
            return false;
        }

        spdlog::info("Sent: {}", message);
        return true;
    }

    std::string receiveMessage()
    {
        if (!connected)
        {
            return "";
        }

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived <= 0)
        {
            if (bytesReceived == 0)
            {
                spdlog::info("Server disconnected");
            }
            else
            {
                spdlog::error("Receive failed: {}", WSAGetLastError());
            }
            connected = false;
            return "";
        }

        std::string message(buffer, bytesReceived);
        spdlog::info("Received: {}", message);
        return message;
    }

    void testLogin(const std::string& username, const std::string& password)
    {
        spdlog::info("\n=== Testing Login ===");
        spdlog::info("Username: {}", username);
        spdlog::info("Password: {}", password);

        // 发送登录请求
        std::string loginRequest = "LOGIN:" + username + ":" + password;
        if (sendMessage(loginRequest))
        {
            // 接收响应
            std::string response = receiveMessage();
            if (!response.empty())
            {
                // 分析响应
                if (response.find("SUCCESS") != std::string::npos)
                {
                    spdlog::info("✓ Login successful!");
                }
                else if (response.find("ERROR") != std::string::npos)
                {
                    spdlog::error("✗ Login failed: {}", response);
                }
                else
                {
                    spdlog::warn("? Unknown response: {}", response);
                }
            }
        }
    }

    void runInteractiveTest()
    {
        spdlog::info("\n=== Interactive Client Test ===");
        spdlog::info("Commands:");
        spdlog::info("  login <username> <password> - Test login");
        spdlog::info("  test - Run predefined tests");
        spdlog::info("  quit - Exit");

        std::string command;
        while (connected && std::getline(std::cin, command))
        {
            if (command == "quit")
            {
                break;
            }
            else if (command.substr(0, 5) == "login")
            {
                // 解析登录命令
                size_t firstSpace = command.find(' ');
                if (firstSpace != std::string::npos)
                {
                    size_t secondSpace = command.find(' ', firstSpace + 1);
                    if (secondSpace != std::string::npos)
                    {
                        std::string username = command.substr(firstSpace + 1, secondSpace - firstSpace - 1);
                        std::string password = command.substr(secondSpace + 1);
                        testLogin(username, password);
                    }
                    else
                    {
                        spdlog::info("Usage: login <username> <password>");
                    }
                }
            }
            else if (command == "test")
            {
                runPredefinedTests();
            }
            else
            {
                spdlog::warn("Unknown command. Type 'help' for available commands.");
            }
        }
    }

    void runPredefinedTests()
    {
        spdlog::info("\n=== Running Predefined Tests ===");

        // 测试有效的登录
        testLogin("admin", "123456");
        std::this_thread::sleep_for(std::chrono::seconds(1));

        testLogin("test", "test123");
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 测试无效的登录
        testLogin("admin", "wrongpassword");
        std::this_thread::sleep_for(std::chrono::seconds(1));

        testLogin("nonexistent", "password");
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 测试格式错误的请求
        testLogin("malformed", "request");
        std::this_thread::sleep_for(std::chrono::seconds(1));

        spdlog::info("\n=== Predefined Tests Complete ===");
    }
};

int main(int argc, char* argv[])
{
    std::string serverHost = "localhost";
    int serverPort = 8080;

    // 解析命令行参数
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--host")
        {
            if (i + 1 < argc)
            {
                serverHost = argv[i + 1];
                i++;
            }
        }
        else if (arg == "-p" || arg == "--port")
        {
            if (i + 1 < argc)
            {
                serverPort = std::stoi(argv[i + 1]);
                i++;
            }
        }
        else if (arg == "-t" || arg == "--test")
        {
            // 运行自动测试
            ClientTest client(serverHost, serverPort);
            if (client.connectToServer())
            {
                client.runPredefinedTests();
            }
            return 0;
        }
    }

    spdlog::info("Account Server Client Test");
    spdlog::info("Server: {}:{}", serverHost, serverPort);

    ClientTest client(serverHost, serverPort);
    
    if (client.connectToServer())
    {
        client.runInteractiveTest();
    }
    else
    {
        spdlog::error("Failed to connect to server. Exiting.");
        return 1;
    }

    return 0;
}