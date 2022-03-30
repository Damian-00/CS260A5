#include "networking.hpp"
#include <iostream>
#include <cstring>
#include <csignal>

namespace CS260 
{
#ifdef WIN32

    void NetworkCreate()
    {
        WSAData initializationData;
        int errorCode = WSAStartup(MAKEWORD(2, 2), &initializationData);
        if (errorCode != 0)
            throw std::runtime_error("Error initializing WINSOCK");
    }

    void NetworkDestroy()
    {
        int errorCode = WSACleanup();
        if (errorCode != 0)
            throw std::runtime_error("Error cleaning WINSOCK");
    }

    /**
     * @brief
     *  Sets whether a socket would be blocking or not
     */
    void SetSocketBlocking(SOCKET fd, bool blocking)
    {
        unsigned long mode = 0;
        if (!blocking)
            mode = 1;
        int errorCode = ioctlsocket(fd, FIONBIO, &mode);      
        if(errorCode == SOCKET_ERROR)
            throw std::runtime_error("Error setting blocking socket: " + std::to_string(WSAGetLastError()));
    }


#else

    void NetworkCreate()
    {
        signal(SIGPIPE, SIG_IGN);
        struct sigaction act = { SIG_IGN, {}, {}, {} };
        sigaction(SIGPIPE, &act, NULL); // Ignore Broken Pipe Signals, get error write instead
    }

    void NetworkDestroy() 
    {
    }

    /**
     * @brief
     *  Sets whether a socket would be blocking or not
     * @param fd
     * @param blocking
     */
    void SetSocketBlocking(SOCKET fd, bool blocking)
    {
        // Retrieve current socket flags
        auto flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            throw std::runtime_error("Could not retrieve socket flags");
        }

        // Add flag
        if (blocking) {
            flags &= ~O_NONBLOCK;
        } else {
            flags |= O_NONBLOCK;
        }

        if (fcntl(fd, F_SETFL, flags) != 0) {
            throw std::runtime_error("Could not set socket blocking flag");
        }
    }

#endif

    in_addr ToIpv4(std::string const& addr)
    {
        in_addr result;
        int errorCode = inet_pton(AF_INET, addr.c_str(), &result);
        if (errorCode != 1)
            throw std::runtime_error("Error converting to IPV4 ");
        return result;
    }

    std::string ToString(in_addr const& addr)
    {
        char result[INET_ADDRSTRLEN];

        if (inet_ntop(AF_INET, &addr, result, INET_ADDRSTRLEN) == NULL)
            throw std::runtime_error("Could not convert IPV4 to string");

        return result;
    }
   

}