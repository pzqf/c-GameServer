#pragma once

// 跨平台socket类型定义
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef _WINSOCKAPI_
        #define _WINSOCKAPI_
    #endif
    
    #include <winsock2.h>
    #include <windows.h>
    #include <string>
    
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #define SOCKET_ERROR_VAL SOCKET_ERROR
    #define CLOSE_SOCKET(sock) closesocket(sock)
    #define GET_LAST_ERROR() std::to_string(WSAGetLastError())
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <string>
    
    typedef int socket_t;
    #define INVALID_SOCKET_VALUE -1
    #define SOCKET_ERROR_VAL -1
    #define CLOSE_SOCKET(sock) close(sock)
    #define GET_LAST_ERROR() std::string(strerror(errno))
#endif