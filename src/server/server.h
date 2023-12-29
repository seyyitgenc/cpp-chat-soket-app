#pragma once

// TODO: implement log macro or something like that

#include <cstring>
#include <unordered_map>
#include <vector>
#include "globals.h"

#define MAX_CLIENTS FD_SETSIZE

#define DEFAULT_SEND_BUF_LEN    512
#define DEFAULT_RECV_BUF_LEN    1024


struct DataBuffer {
public:
    char    buf[DEFAULT_RECV_BUF_LEN];
    int     dataSize;
    int     sendOffset;
    bool    isNewData;
};

struct ServerContext {
public:
    DataBuffer recdData;
    SOCKET sock;
    BYTE addrFamily;

    const char *pInterface;
    const char *pPort;

    std::vector<std::pair<std::string,SOCKET>> clientList;
};

class Server
{
public:
    Server() = default;
    ~Server() = default;
public:
    bool initEverything(int argc, char **argv);
    void run();
    void close();

private:
    bool parseArgs(int argc, char **argv);
    bool initCore();
    bool initListeningSockets();

private:
    bool isRunning();
    // bool sendAllTarget(); // sends message to all clients. this will iterate over client list
    // bool sendOneTarget(Client target); // note: this will take an argument

private:
    void connectionHandler(SOCKET clientSock);
    void acceptHandler();
    void sendHandler();

private: // helpers
    void printHelp();
    void printServerInfo();
    void printAddressString(LPSOCKADDR pSockAddr, DWORD dwSockAddrLen);
private:
#ifdef _WIN64
    WSAData _wsaData;
#endif
    bool _bInitCore;
    bool _bInitSocket;
    bool _bParseArgs;
    bool _bIsRunning;
    ServerContext _Context;
};