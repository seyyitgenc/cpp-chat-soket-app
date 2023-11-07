#include "client.h"
#include <thread>

std::mutex mtx; // mutex for critical section
// FIXME: there is a deadlock bug can implement multithreaded approach

// ---------------
// init everything
// ---------------
bool Client::initEverything(int argc, char **argv){
    parseArgs(argc, argv) ? 
        printf("Arguments parsed successfully!\n") :
        printf("Argument parsing failed!\n");
    
    initCore() ?
        printf("Client core initialized successfully!\n") :
        printf("Client core initialization failed!\n");
    
    initSocket() ? 
        printf("Client socket initialized successfully!\n") :
        printf("Client socket initialization failed!\n");

    return _bInitCore && _bInitSocket && _bParseArgs;
}
// ---------------------------------
// parse the arguments given by user
// ---------------------------------
bool Client::parseArgs(int argc, char **argv){
    //! this is a easy version of a argument parser
    //! it's only supports the following arguments:
    //! -s <server> -p <port>
    // todo: later on implement these aswell : -d <delay> -l <send_buf_len> -r <recv_buf_len>
    printf("Parsing arguments...\n");
    
    _Context.addr_family    =   DEFAULT_ADDR_FAMILY;
    _Context.pServer        =   DEFAULT_SERVER;
    _Context.pPort          =   DEFAULT_PORT;
    _Context.send_buf_len   =   DEFAULT_SEND_BUF_LEN;
    _Context.recv_buf_len   =   DEFAULT_RECV_BUF_LEN;
    _Context.delay          =   DEFAULT_DELAY;

    _bParseArgs = true;

    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "-s") == 0){
            _Context.pServer = argv[++i];
        }
        else if (strcmp(argv[i], "-p") == 0){
            _Context.pPort = argv[++i];
        }
        
        else
        {
            printf("Unknown argument: %s\n", argv[i]);
            _bParseArgs = false;
            break;
        }
    }

    printf("Server: %s\n", _Context.pServer);
    printf("Port: %s\n", _Context.pPort);

    return _bParseArgs;
}

// ---------------------
// init the core
// ---------------------
bool Client::initCore()
{
    _bInitCore = true;

    printf("Initializing client core...\n");
    if (WSAStartup(MAKEWORD(2, 2), &_wsaData) != 0)
        _bInitCore = false;
    // TODO: later on i can initialize the ImGui here
    return _bInitCore;
}

// ---------------------
// init the socket
// ---------------------
bool Client::initSocket()
{
    _bInitSocket = false;
    
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family     =   _Context.addr_family;
    hints.ai_protocol   =   IPPROTO_TCP;
    hints.ai_socktype   =   SOCK_STREAM;

    // this getaddrinfo can return multiple ip adresses
    if (getaddrinfo(_Context.pServer, _Context.pPort, &hints, &result) != 0)
    {
        printf("getaddrinfo failed with Error:: %d\n", WSAGetLastError());
         _bInitSocket = false;
        goto CLEANUP;
    }

    // this will handle multiple ip adresses
    for(ptr = result; ptr != NULL; ptr = ptr->ai_next){

        // note: we can use WSASocket function here aswell
        _Context.sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (_Context.sock == INVALID_SOCKET)
        {
            printf("socket failed with Error:: %d\n", WSAGetLastError());
            printf("Trying next address...\n\n");
            continue;
        }

        printf("Socket created successfully with handle = %d\n", _Context.sock);
        if (connect(_Context.sock, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
        {
            printf("connect failed with Error:: %d\n", WSAGetLastError());
            closesocket(_Context.sock);
            _Context.sock = INVALID_SOCKET;
            continue;
        }
        _bInitSocket = true;
        printf("Connected to server successfully!\n");
        break;
    }

CLEANUP:
    if (result)
        freeaddrinfo(result);

    return _bInitSocket;
}

// ---------------------
// main loop of the app
// ---------------------
bool Client::run()
{
    while (!_bPeerShutdown)
    {
        prepareSendBuf();
        prepareRecvBuf();
        doSendOnce();
        doRecvOnce();
    }
    return false;
}

void Client::close()
{
    if (_bInitCore)
        WSACleanup();
    if (_bInitSocket)
        closesocket(_Context.sock);
}

// ---------------------
// prepares the send buf
// ---------------------
bool Client::prepareSendBuf()
{
    bool bSuccess = false;
    _Context.pSendBuf = (char*) malloc(_Context.send_buf_len + 1);
    if(_Context.pSendBuf == NULL)
        printf("malloc failed.\n");
    else
    {
        memset(_Context.pSendBuf, 'H', _Context.send_buf_len);
        _Context.pSendBuf[_Context.send_buf_len] = '\0'; // null terminate the string
        _Context.nBytesRemainingToBeSent = _Context.send_buf_len;

        bSuccess = true;
    }
    printf("Send buffer prepared.\n");
    return bSuccess;
}

// ---------------------
// frees the send buffer
// ---------------------
void Client::freeSendBuf()
{
    if (_Context.pSendBuf != NULL)
    {
        free(_Context.pSendBuf);
        printf("Freed send buffer.\n");
        _Context.pSendBuf = NULL;
    }
}

// ---------------------
// prepares the recv buf
// ---------------------
bool Client::prepareRecvBuf()
{
    bool bSuccess = false;
    _Context.pRecvBuf = (char*) malloc(_Context.recv_buf_len + 1);

    if (_Context.pRecvBuf == NULL)
        printf("malloc failed at prepareRecvBuf.\n"); // maybe we can asser ??
    else
    {
        memset(_Context.pRecvBuf, 0, _Context.recv_buf_len + 1);
        _Context.nBytesRecvd = 0;

        bSuccess = true;
    }
    
    return bSuccess;
}

// ---------------------
// frees the recv buffer
// ---------------------
void Client::freeRecvBuf()
{
    if(_Context.pRecvBuf != NULL){
        free(_Context.pRecvBuf);
        printf("Freed recv buffer.\n");
        _Context.pRecvBuf = NULL;
    }
}

// ---------------------------------------------------
// this function will try to send our data in one pass
// ---------------------------------------------------
int Client::doSendOnce(){
    int nBytesSent = 0;
    int startPosition = _Context.send_buf_len - _Context.nBytesRemainingToBeSent;

    nBytesSent = send(_Context.sock, _Context.pSendBuf + startPosition, _Context.nBytesRemainingToBeSent, 0);

    if(nBytesSent != SOCKET_ERROR){
        _Context.nBytesRemainingToBeSent -= nBytesSent;
    }
    else
       return WSAGetLastError();
    return 0;
}

// -----------------------------------------------------------------
// this functions will recieve all data at once
// -----------------------------------------------------------------
int Client::doRecvOnce(){
    printf("Recieving data...\n");
    int nbytesRecv = recv(_Context.sock, _Context.pRecvBuf, _Context.recv_buf_len, 0);

    if(nbytesRecv == SOCKET_ERROR)
        return WSAGetLastError();
    else{
        _Context.nBytesRecvd += nbytesRecv; // fixme: later on i can remove this
        printf("server says : %s\n",_Context.pRecvBuf);
        printf("Recieved %d bytes so far\n", _Context.nBytesRecvd);
    }
    return nbytesRecv;
}

// ------------------------------------------------------------------------------------------
// note: this is a half close
// note: this prevents client sending data to server but server can still send data to client
// ------------------------------------------------------------------------------------------
void Client::doShutDown(){
    if(shutdown(_Context.sock, SD_SEND) == SOCKET_ERROR)
        printf("shutdown failed with Error:: %d\n", WSAGetLastError());
    else
        printf("shutdown successful!\n");
}

// ---------------------------------------------------------
// Client will accept data only once and send data only once
// ---------------------------------------------------------
void Client::doSendThenRecv(){
    
    doSendUntilDone();
    // doShutDown();
    doRecvUntilDone();
}


// ---------------------------------------------------------
// Client will accept data until there is no more data left
// ---------------------------------------------------------
void Client::doSendUntilDone(){
    printf("Sending data...\n");
    int retries = 0;
    const int maxRetries = 5;
    do
    {
        int err = doSendOnce();
        switch (err)
        {
        case 0: // all data is sent
            printf("all bytes send successfully!\n");
            return;
    
        case WSAEWOULDBLOCK: // our data cannot fin it the send buffer
            printf("send buffer is full, waiting for a while...\n");
            Sleep(_Context.delay);
            retries++;
            if (retries >= maxRetries){
                printf("Maximun retries reached, aborting...\n");
                return;
            }
            break;
        case WSAECONNRESET:
            printf("Send returned WSAECONNRESET. Remote socket mush have been reseted by peer.\n");
            _bPeerShutdown = true;
            return;
        default: // other errors
            printf("send failed with Error:: %d\n", err);
            return;
        }
    } while (1);
}

void Client::doRecvUntilDone(){
    printf("Waiting for data...\n");
    int err;
    int totalBytesReceived = 0;
    const int maxBytes = 10000; // adjust this as needed
    do
    {
        int bytesReceived = doRecvOnce();
        switch (bytesReceived)
        {
            case 0: // remote socket has been closed
                printf("Recv returned 0. Remote socket must have been"
                "closed.\n");
                return;

            case SOCKET_ERROR: // error occured
                err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) // recv buffer is empty
                {
                    printf("recv buffer is empty, waiting for a while...\n");
                    Sleep(_Context.delay);
                }
                else{
                    printf("ERROR:: recv returned%d\n",err);
                    return;
                }
                break;
            case WSAECONNRESET: // remote socket has been shutdown
                printf("Recv returned WSAECONNRESET. Remote socket must have been reseted by peer.\n");
                _bPeerShutdown = true;
                return;
            default: // > 0  bytes recieved
                totalBytesReceived += bytesReceived;
                if (totalBytesReceived >= maxBytes) {
                    printf("Maximum number of bytes received. Exiting...\n");
                    return;
                }
                break;
        }
    } while (1);
}