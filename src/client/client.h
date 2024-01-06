#pragma once

#include "globals.h"

#define DEFAULT_SEND_BUF_LEN    32
#define DEFAULT_RECV_BUF_LEN    10240
#define DEFAULT_DELAY           1000
#define DEFAULT_USERNAME        "anon"

enum ClientType{
    SENDER,
    RECIEVER,
    NONE
};

class ClientContext{
public:
    std::string sendBuf;
    std::string recvBuf;
    
    BYTE        addrFamily;
    const char  *pServer;
    const char  *pPort;

    SOCKET      sock;
    std::string userName;
};

class Client
{
public:
    Client() : _bPeerShutdown(false),_bSocketError(false) {};
    ~Client() = default;

public:     // general functions that user can use
    bool    initEverything(int argc, char** argv);
    void    run();
    void    close();

private:    // threads
    void    sendHandler();
    void    recvHandler();

private:    // Message helpers
    void    parseMessage(ClientType);
    void    sendMessage();
    void    recvMessage();
    
    void    generateErrorBits(std::string);
    std::string simpleParityCheck(std::string msg);
    std::string cyclicRedundancyCheck(std::string msg);
    
private:    // init functions
    bool    initCore();
    bool    initSocket();
    bool    parseArgs(int argc, char** argv);

private:    // print functions
    void    printClientInfo();
    void    printHelp();

private:
    WSADATA         _wsaData;
    ClientContext   _Context;
    bool            _bInitCore;
    bool            _bInitSocket;
    bool            _bParseArgs;
    bool            _bPeerShutdown;
    bool            _bSocketError;
};