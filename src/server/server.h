#pragma once

// TODO: implement log macro or something like that
#include <vector>
#include "globals.h"

#define MAX_CLIENTS FD_SETSIZE

struct DataBuffer {
public:
    char  *pSendBuf;
    char  *pRecvBuf;
    int dataSize;
    int sendOffset;
    bool isNewData;
};

struct ClientList{
public:
    SOCKET sock;
    DataBuffer db;
    
};

struct ServerContext {
public:
    SOCKET sock;
    BYTE addrFamily;
    const char *pInterface;
    const char *pPort;
    std::vector<ClientList> cList; // this will hold clients
    // todo : implement multiple listening sockets
    //! so what we need to do is create a struct that holds general socket info 
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
    bool isRunning();
    // bool sendAllTarget(); // sends message to all clients. this will iterate over client list
    // bool sendOneTarget(Client target ); // note: this will take an argument 
    
    void acceptWorker();
    void sendWorker();
private:
    WSAData _wsaData;
    bool _bInitCore;
    bool _bInitSocket;
    bool _bParseArgs;
    bool _bIsRunning;
    ServerContext _Context;
};