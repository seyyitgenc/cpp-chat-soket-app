#include "server.h"
#include <thread>

bool Server::initEverything(int argc, char **argv){
    _bIsRunning = false;
    parseArgs(argc, argv) ?
        printf("Arguments parsed successfully!\n") :
        printf("Argument parsing failed!\n");

    initCore() ?
        printf("Server core initialized successfully!\n") :
        printf("Server core initialization failed!\n");

    initListeningSockets() ?
        printf("Server socket initialized successfully!\n") :
        printf("Server socket initialization failed!\n");

    if (_bParseArgs && _bInitCore && _bInitSocket)
        _bIsRunning = true;
    
    return _bIsRunning;
}

bool Server::parseArgs(int argc, char **argv){
    printf("Parsing Arguments...\n");

    _Context.addrFamily =   DEFAULT_ADDR_FAMILY;
    _Context.pInterface =   DEFAULT_SERVER;
    _Context.pPort      =   DEFAULT_PORT;
    
    _bParseArgs = true;

    for (size_t i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-s") == 0)
            _Context.pInterface = argv[++i];
        else if(strcmp(argv[i], "-p") == 0)
            _Context.pPort = argv[++i];
        else{
            printf("Unknown argument: %s\n", argv[i]);
            _bParseArgs = false;
            break;
        }
    }
    printf("Server: %s\n", _Context.pInterface);
    printf("Port: %s\n", _Context.pPort);

    return _bParseArgs;
}

bool Server::initCore(){
    _bInitCore = true;
    printf("Initializing client core...\n");
    if (WSAStartup(MAKEWORD(2,2), &_wsaData) != 0)
        _bInitCore = false;
    return _bInitCore;
}

bool Server::initListeningSockets(){
    _bInitSocket = false;

    unsigned long nonBlocking = 1;

    SOCKET newSock;
    struct addrinfo *result = nullptr, *ptr = nullptr, hints;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family     =    _Context.addrFamily;
    hints.ai_socktype   =   SOCK_STREAM;
    hints.ai_protocol   =   IPPROTO_TCP;  
    hints.ai_flags      =   AI_PASSIVE;

    if (getaddrinfo(_Context.pInterface, _Context.pPort, &hints, &result) != 0);
    {
        printf("getaddrinfo failed. Error = %d\n", WSAGetLastError());
        return _bInitSocket;
    }
    if (result == nullptr)
    {
        printf("getaddrinfo returned res = nullptr\n");
        return _bInitSocket;
    }

    newSock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (newSock == INVALID_SOCKET)
    {
        printf("socket failed. Error = %d\n", WSAGetLastError());
        printf("Ignoring this address and continuing with the next. \n\n");
    }

    printf("Created socket with handle = %d\n", newSock);

    if (bind(newSock, ptr->ai_addr, ptr->ai_addrlen))
    {
        printf("bind failed. Error = %d\n",WSAGetLastError());
        closesocket(newSock);
        return _bInitSocket;
    }

    printf("Socket Bound Succesfully\n");

    if (listen(newSock, MAX_CLIENTS) != 0)
    {
        printf("listen failed. Error = %d\n", WSAGetLastError());
        closesocket(newSock);
        return _bInitSocket;
    }

    printf("Listen Succesfull\n");

    // for non-blocking select we need to explicitily make the socket non-blocking
    if (ioctlsocket(newSock, FIONBIO, &nonBlocking) == SOCKET_ERROR)
        printf("Can't put socket into non-blocking mode. Error = %d\n", WSAGetLastError());

    if (result)
        freeaddrinfo(result);
    
    printf("Exiting CreateListeningSockets\n");   
    _Context.sock = newSock;

    _bInitSocket = true;
    
    return _bInitSocket;
}

void Server::acceptWorker(){}

void Server::sendWorker(){}

void Server::run(){
    std::thread acceptThread = std::thread(&Server::acceptWorker,this);
    acceptThread.detach();

    std::thread sendThread = std::thread(&Server::sendWorker,this);
    sendThread.join(); 
    // this line will not printed until sendThread finishes
    printf("Program terminated\n");
}

bool Server::isRunning(){
    return _bIsRunning;
}

void Server::close(){

}