#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <stdio.h>
#include <iostream>
#include <string>

#define DEFAULT_ADDR_FAMILY     AF_INET
#define DEFAULT_SERVER          "localhost"
#define DEFAULT_PORT            "27015"
#define DEFAULT_SEND_BUF_LEN    32
#define DEFAULT_RECV_BUF_LEN    10240
#define DEFAULT_DELAY           1000

typedef struct _ClientContext{
    BYTE        addr_family;
    const char  *pServer;
    const char  *pPort;
    char        *pSendBuf;
    char        *pRecvBuf;
    int         send_buf_len;
    int         recv_buf_len;
    int         delay;
    int         nBytesRemainingToBeSent;
    int         nBytesRecvd;
    SOCKET      sock;
} ClientContext, *PClientContext;

class Client
{
public:
    Client(): _bPeerShutdown(false){};
    ~Client() = default;

public:     // general functions that user can use
    bool    initEverything(int argc, char** argv);
    bool    run();
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

private:    // init functions
    bool    initCore();
    bool    initSocket();
    bool    parseArgs(int argc, char** argv);

private:
    WSADATA         _wsaData;
    ClientContext   _Context;
    bool            _bInitCore;
    bool            _bInitSocket;
    bool            _bParseArgs;
    bool            _bPeerShutdown;
};