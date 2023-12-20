#include "client.h"

#include <thread>
#include <random>
#include <regex>
#include <limits>

// todo : add assert for initialization functions

// ---------------
// init everything
// ---------------
bool Client::initEverything(int argc, char **argv)
{
    initCore() ?
        std::cout << "Client core initialized successfully!\n" :
        std::cout << "Client core initialization failed!\n";

    parseArgs(argc, argv) ?
        std::cout << "Arguments parsed successfully!\n" :
        std::cout << "Argument parsing failed!\n";

    if (_bParseArgs)
        initSocket() ?
        std::cout << "Client socket initialized successfully!\n" :
        std::cout << "Client socket initialization failed!\n";

    return _bInitCore && _bInitSocket && _bParseArgs;
}

// -------------------------
// simple argument validator
// -------------------------
bool isValidArgParameter(char *arg)
{
    std::basic_regex reg("[^a-zA-Z0-9]");
    // if given argument is contains special chars it will fail
    if (std::regex_search(arg,reg))
        return false;
    return true;
}

// ---------------------------------
// parse the arguments given by user
// ---------------------------------
bool Client::parseArgs(int argc, char **argv)
{
    //! this is a easy version of a argument parser
    //! it's only supports the following arguments:
    //! -s <server> -p <port> -u <username> -h for help
    // todo: later on implement these aswell : -d <delay> -l <sendBufLen> -r <recvBufLen>
    std::cout << "Parsing arguments...\n";

    _bParseArgs = true;

    _Context.addrFamily     =   DEFAULT_ADDR_FAMILY;
    _Context.pServer        =   DEFAULT_SERVER;
    _Context.pPort          =   DEFAULT_PORT;
    _Context.sendBufLen     =   DEFAULT_SEND_BUF_LEN;
    _Context.recvBufLen     =   DEFAULT_RECV_BUF_LEN;
    _Context.delay          =   DEFAULT_DELAY;

    // note : this is not reliable solution
    // randomized username
    std::random_device rd;
    std::uniform_int_distribution<int> dist(1,1000);
    _Context.userName = DEFAULT_USERNAME + std::to_string(dist(rd));

    for (int i = 1; i < argc; i++){
        char firstChar = argv[i][0];
        // make sure first char starts with '-'
        if (!(firstChar == '-'))
        {
            std::cout << "ERROR: Parsing failed! Option has to begin with '-' : " <<  argv[i] << std::endl;
            _bParseArgs = false;
            break;
        }
        switch (argv[i][1])
        {
        case 's':
            if (i + 1 >= argc)
            {
                std::cout << "ERROR: Parsing failed! Server name needed for -s option.\n";
                printHelp();
                _bParseArgs = false;
            }
            else
                i++;
            if (isValidArgParameter(argv[i]))
                _Context.pServer = argv[i];
            else{
                std::cout << "ERROR: Parsing failed! Server name needed for -s option.\n";
                _bParseArgs = false;
            }
            break;
        case 'p':
            if (i + 1 >= argc)
            {
                std::cout << "ERROR: Parsing failed! Port number needed for -p option.\n";
                printHelp();
                _bParseArgs = false;
            }
            else
                i++;
            if (isValidArgParameter(argv[i]))
                _Context.pPort = argv[i];
            else{
                _bParseArgs = false;
                std::cout << "ERROR: Parsing failed. Port number needed for -p option.";
            }
            break;
        case 'u':
            if (i + 1 >= argc)
            {
                std::cout   << "ERROR: Parsing failed! User name needed for -u option.\n";
                _bParseArgs = false;
            }
            else
                i++;
            if (isValidArgParameter(argv[i]))
                _Context.userName = argv[i];
            else{
                _bParseArgs = false;
                std::cout << "ERROR: Parsing failed. User name needed for -u option.";
            }
            break;
        // note: temporary solution
        case 'h':
            printHelp();
            _bParseArgs = false;
            break;
        case '?':
            printHelp();
            _bParseArgs = false;
            break;
        default:
            break;
        }
    }
    printClientInfo();
    return _bParseArgs;
}

// ---------------------
// init the core
// ---------------------
bool Client::initCore()
{
    _bInitCore = true;

    std::cout << "Initializing client core...\n";
    if (WSAStartup(MAKEWORD(2, 2), &_wsaData) != 0)
        _bInitCore = false;
    // todo: init IMGui here
    return _bInitCore;
}

// ---------------------
// init the socket
// ---------------------
bool Client::initSocket()
{
    _bInitSocket = false;

    struct addrinfo *result = nullptr, *ptr = nullptr, hints;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family     =   _Context.addrFamily;
    hints.ai_protocol   =   IPPROTO_TCP;
    hints.ai_socktype   =   SOCK_STREAM;

    // this getaddrinfo can return multiple ip adresses
    if (getaddrinfo(_Context.pServer, _Context.pPort, &hints, &result) != 0)
    {
        std::cout << "getaddrinfo failed with Error:: " <<  WSAGetLastError() << std::endl;
        _bInitSocket = false;
        if (result)
            freeaddrinfo(result);
        return _bInitSocket;
    }

    // this will handle multiple ip adresses
    for(ptr = result; ptr != nullptr; ptr = ptr->ai_next){
        _Context.sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (_Context.sock == INVALID_SOCKET)
        {
            std::cout << "socket failed with Error:: " <<  WSAGetLastError() << std::endl;
            std::cout << "Trying next address...\n\n";
            continue;
        }

        std::cout << "Socket created successfully with handle = " <<  _Context.sock << std::endl;
        if (connect(_Context.sock, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
        {
            std::cout << "connect failed with Error:: " <<  WSAGetLastError() << std::endl;
            closesocket(_Context.sock);
            _Context.sock = INVALID_SOCKET;
            continue;
        }
        _bInitSocket = true;
        std::cout << "Connected to server successfully!\n";
        break;
    }

    if (result)
        freeaddrinfo(result);

    return _bInitSocket;
}

// ---------------------
// main loop of the app
// ---------------------
void Client::run()
{

    auto iSend = send(_Context.sock, _Context.userName.c_str(), strlen(_Context.userName.c_str()), 0);
    if (iSend == SOCKET_ERROR)
    {
        std::cout << "Sending Failed " << "\n";
        std::cout << "Error No-> " << WSAGetLastError() << "\n";
        return;
    }

    std::thread sendThread = std::thread(&Client::sendHandler,this);
    sendThread.detach(); // detach from current thread

    std::thread recvThread = std::thread(&Client::recvHandler,this);
    recvThread.join(); // join to current

    std::cout << "Client Shutting Down...! \n";
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
   
    
    // bool bSuccess = false;
    // _Context.pSendBuf = (char*) malloc(_Context.sendBufLen + 1);
    // if(_Context.pSendBuf == nullptr)
    //     std::cout << "malloc failed.\n";
    // else
    // {
    //     memset(_Context.pSendBuf, 'H', _Context.sendBufLen);
    //     _Context.pSendBuf[_Context.sendBufLen] = '\0'; // nullptr terminate the string
    //     _Context.nBytesRemainingToBeSent = _Context.sendBufLen;
    //     bSuccess = true;
    // }
    // std::cout << "Send buffer prepared.\n";
    // return bSuccess;
    return true;
}

// ---------------------
// frees the send buffer
// ---------------------
void Client::freeSendBuf()
{
    // if (_Context.pSendBuf != nullptr)
    // {
    //     free(_Context.pSendBuf);
    //     std::cout << "Freed send buffer.\n";
    //     _Context.pSendBuf = nullptr;
    // }
}

// ---------------------
// prepares the recv buf
// ---------------------
bool Client::prepareRecvBuf()
{
    bool bSuccess = false;
    _Context.pRecvBuf = (char*) malloc(_Context.recvBufLen + 1);

    if (_Context.pRecvBuf == nullptr)
        std::cout << "malloc failed at prepareRecvBuf.\n"; // maybe we can assert here ?
    else
    {
        memset(_Context.pRecvBuf, 0, _Context.recvBufLen + 1);
        _Context.nBytesRecvd = 0;
        bSuccess = true;
    }
    std::cout << "Recv buffer prepared.\n";
    return bSuccess;
}

// ---------------------
// frees the recv buffer
// ---------------------
void Client::freeRecvBuf()
{
    if(_Context.pRecvBuf != nullptr){
        free(_Context.pRecvBuf);
        std::cout << "Freed recv buffer.\n";
        _Context.pRecvBuf = nullptr;
    }
}

// ---------------------------------------------------
// this function will try to send our data in one pass
// ---------------------------------------------------
int Client::doSendOnce(){
    int nBytesSent = 0;
    int startPosition = _Context.sendBufLen - _Context.nBytesRemainingToBeSent;

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
int Client::doRecvOnce()
{
    std::cout << "Recieving data...\n";
    int nbytesRecv = recv(_Context.sock, _Context.pRecvBuf, _Context.recvBufLen, 0);

    if(nbytesRecv == SOCKET_ERROR)
        return WSAGetLastError();
    else{
        _Context.nBytesRecvd += nbytesRecv; // fixme: later on i can remove this
        std::cout << "server says : %s\n",_Context.pRecvBuf;
        std::cout << "Recieved %d bytes so far\n", _Context.nBytesRecvd;
    }
    return nbytesRecv;
}

// ------------------------------------------------------------------------------------------
// note: this is a half close
// note: this prevents client sending data to server but server can still send data to client
// ------------------------------------------------------------------------------------------
void Client::doShutDown()
{
    if(shutdown(_Context.sock, SD_SEND) == SOCKET_ERROR)
        std::cout << "shutdown failed with Error:: %d\n", WSAGetLastError();
    else
        std::cout << "shutdown successful!\n";
}

// -----------------------------------------------
// Handler for multi threaded approach to send data
// -----------------------------------------------
void Client::sendHandler()
{
    // todo: add terminate flag
    std::string data;
    do
    {
        std::cout << _Context.userName << " : ";
        std::getline(std::cin, data);
        if (data.size() > 0)
        {
            data = _Context.userName + ": " + data;
            std::cout << "\nI am sending this data :" << data << std::endl;
            if (send(_Context.sock, data.c_str(), strlen(data.c_str()), 0) == SOCKET_ERROR)
            {
                std::cout << "send failed: " << WSAGetLastError() << "\n";
                close();
                return;
            }
        }
    } while (data.size() > 0);
};

// -----------------------------------------------
// Handler for multi threaded approach to recv data
// -----------------------------------------------
void Client::recvHandler()
{
    while (true)
    {
        char recvbuf[512];
        ZeroMemory(recvbuf, 512);
        auto iRecv = recv(_Context.sock, recvbuf, 512, 0);
        if (iRecv > 0)
        {
            std::cout << '\r';
            std::cout << recvbuf << "\n";
        }
        else if (iRecv == 0) {
            std::cout << "Connection closed" << "\n";
            return;
        }
        else {
            std::cout << "recv failed: " << WSAGetLastError() << "\n";
            return;
        }
    }
};

// ---------------------------------------------------------
// Client will accept data only once and send data only once
// ---------------------------------------------------------
void Client::doSendThenRecv()
{
    doSendUntilDone();
    // doShutDown();
    doRecvUntilDone();
}

// ---------------------------------------------------------
// Client will accept data until there is no more data left
// ---------------------------------------------------------
void Client::doSendUntilDone()
{
    std::cout << "Sending data...\n";
    int retries = 0;
    const int maxRetries = 5;
    // todo: set a flag here to terminate the program
    do
    {
        int err = doSendOnce();
        switch (err)
        {
        case 0: // all data is sent
            std::cout << "all bytes send successfully!\n";
            return;

        case WSAEWOULDBLOCK: // our data cannot fin it the send buffer
            std::cout << "send buffer is full, waiting for a while...\n";
            Sleep(_Context.delay);
            retries++;
            if (retries >= maxRetries){
                std::cout << "Maximun retries reached, aborting...\n";
                return;
            }
            break;
        case WSAECONNRESET:
            std::cout << "Send returned WSAECONNRESET. Remote socket mush have been reseted by peer.\n";
            _bPeerShutdown = true;
            return;
        default: // other errors
            std::cout << "send failed with Error: " << err << std::endl;
            return;
        }
    } while (1);
}

void Client::doRecvUntilDone()
{
    std::cout << "Waiting for data...\n";
    int err;
    int totalBytesReceived = 0;
    const int maxBytes = 10000; // adjust this as needed
    do
    {
        int bytesReceived = doRecvOnce();
        switch (bytesReceived)
        {
            case 0: // remote socket has been closed
                std::cout << "Recv returned 0. Remote socket must have been"
                "closed.\n";
                return;

            case SOCKET_ERROR: // error occured
                err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) // recv buffer is empty
                {
                    std::cout << "recv buffer is empty, waiting for a while...\n";
                    Sleep(_Context.delay);
                }
                else{
                    std::cout << "ERROR:: recv returned%d\n",err;
                    return;
                }
                break;
            case WSAECONNRESET: // remote socket has been shutdown
                std::cout << "Recv returned WSAECONNRESET. Remote socket must have been reseted by peer.\n";
                _bPeerShutdown = true;
                return;
            default: // > 0  bytes recieved
                totalBytesReceived += bytesReceived;
                if (totalBytesReceived >= maxBytes) {
                    std::cout << "Maximum number of bytes received. Exiting...\n";
                    return;
                }
                break;
        }
    } while (1);
}

// ------------------
// Prints server info
// ------------------
void Client::printClientInfo()
{
    std::cout
            << "\n"
            << "            Client Info\n"
            << "----------------------------------\n"
            << "Server : "  << _Context.pServer   << "\n"
            << "Port   : "  << _Context.pPort     << "\n"
            << "Delay  : "  << _Context.delay     << "\n"
            << "User   : "  << _Context.userName  << "\n"
            << "----------------------------------\n";
}

// ---------------------------
// Prints how to use arguments
// ---------------------------
void Client::printHelp()
{
    std::cout
            << "\n----------------------------------------------------\n"
            << "for server name : -s <server> (by default localhost)\n"
            << "for port number : -p <port> (by default 27015)\n"
            << "for user name   : -u <username> (by default anon)\n"
            << "for help -h or -?\n"
            << "----------------------------------------------------\n";
}