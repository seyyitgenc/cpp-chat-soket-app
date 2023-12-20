#pragma once

#include "globals.h"

#include <string>

#define DEFAULT_SEND_BUF_LEN    32
#define DEFAULT_RECV_BUF_LEN    10240
#define DEFAULT_DELAY           1000
#define DEFAULT_USERNAME        "astolfo"

class ClientContext{
public:
    BYTE        addrFamily;
    const char  *pServer;
    const char  *pPort;
    char        pSendBuf[DEFAULT_SEND_BUF_LEN];
    char        *pRecvBuf;
    int         sendBufLen;
    int         recvBufLen;
    int         delay;
    int         nBytesRemainingToBeSent;
    int         nBytesRecvd;
    SOCKET      sock;
    std::string userName;
};

class Client
{
public:
    Client() : _bPeerShutdown(false){};
    ~Client() = default;

public:     // general functions that user can use
    bool    initEverything(int argc, char** argv);
    void    run();
    void    close();

private:    // buffer management functions
    void    freeSendBuf();
    bool    prepareSendBuf();
    bool    prepareRecvBuf();
    void    freeRecvBuf();

private:     // send and recive types
    void    doSendThenRecv();

private:    // send and recive functions
    int     doSendOnce();
    int     doRecvOnce();
    void    doSendUntilDone();
    void    doRecvUntilDone();
    void    doShutDown();

private:    // threads
    void    sendHandler();
    void    recvHandler();

private:    // init functions
    bool    initCore();
    bool    initSocket();
    bool    parseArgs(int argc, char** argv);
    void    printClientInfo();
    void    printHelp();
private:
    WSADATA         _wsaData;
    ClientContext   _Context;
    bool            _bInitCore;
    bool            _bInitSocket;
    bool            _bParseArgs;
    bool            _bPeerShutdown;
};