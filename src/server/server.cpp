#include "server.h"

#include <thread>
#include <regex>
#include <deque>

bool Server::initEverything(int argc, char **argv)
{
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

// -------------------------
// simple argument validator
// -------------------------
bool isValidArgParameter(char *arg)
{
    std::regex reg("[^a-zA-Z0-9]");
    if (std::regex_search(arg,reg))
        return false;
    return true;
}
// fixme: when user selects 'h' or '?' arg. application says arg parsing failed.
// ---------------------------------
// parse the arguments given by user
// ---------------------------------
bool Server::parseArgs(int argc, char **argv)
{
    printf("Parsing Arguments...\n");

    _Context.addrFamily =   DEFAULT_ADDR_FAMILY;
    _Context.pInterface =   DEFAULT_SERVER;
    _Context.pPort      =   DEFAULT_PORT;

    _bParseArgs = true;

    for (int i = 1; i < argc; i++)
    {
        char firstChar = argv[i][0];
        if (!(firstChar == '-'))
        {
            std::cout << "ERROR: Parsing failed! Option has to begin with '-' :" << argv[i] << std::endl;
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
                _Context.pInterface = argv[i];
            else{
                std::cout << "ERROR: Parsing failed! Interface  needed for -s option.\n";
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
    printServerInfo();
    return _bParseArgs;
}

// ---------------
// Initialize Core
// ---------------
bool Server::initCore()
{
    _bInitCore = true;
#ifdef _WIN64
    printf("Initializing client core...\n");
    if (WSAStartup(MAKEWORD(2,2), &_wsaData) != 0)
        _bInitCore = false;
    return _bInitCore;
#endif
}

// --------------------------------------------------------------------------------
// todo: add multiple listening socket support 
// Initialize Listening Socket(s) for now it's initialize only one listening socket
// -------------------------------------------------------------------------------
bool Server::initListeningSockets()
{
    _bInitSocket = true;

    unsigned long nonBlocking = 1;

    SOCKET newSock;
    struct addrinfo *result = nullptr, *ptr = nullptr, hints;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family     =    _Context.addrFamily;
    hints.ai_protocol   =   IPPROTO_TCP;
    hints.ai_socktype   =   SOCK_STREAM;
    hints.ai_flags      =   AI_PASSIVE;

    if (getaddrinfo(_Context.pInterface, _Context.pPort, &hints, &result) != 0)
    {
        printf("getaddrinfo failed. Error = %d\n", WSAGetLastError());
        _bInitSocket = false;
        if (result)
            freeaddrinfo(result);
        return _bInitSocket;
    }
    for (ptr = result; ptr != nullptr; ptr = ptr->ai_next)
    {
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
            continue;
        }

        printf("Socket Bound Succesfully\n");

        if (listen(newSock, MAX_CLIENTS) != 0)
        {
            printf("listen failed. Error = %d\n", WSAGetLastError());
            closesocket(newSock);
            continue;
        }
        printf("Listen Succesfull\n");

        // for non-blocking select we need to explicitily make the socket non-blocking
        // if (ioctlsocket(newSock, FIONBIO, &nonBlocking) == SOCKET_ERROR)
        //     printf("Can't put socket into non-blocking mode. Error = %d\n", WSAGetLastError());
        _bInitSocket = true;
        std::cout << "Listening socket created succesfully!\n";
        break;
    }
    if (result)
        freeaddrinfo(result);

    _Context.sock = newSock;
    return _bInitSocket;
}


// ------------------
// Server entry point
// ------------------
void Server::run()
{
    // FD_ZERO(&_Context.readFDSet);
    std::thread acceptThread = std::thread(&Server::acceptHandler,this);
    acceptThread.detach();

    std::thread sendThread = std::thread(&Server::sendHandler,this);
    sendThread.join();
    // this line will not printed until sendThread finishes
    printf("Program terminated\n");
}

// --------------------------------------------------
// Handles the connection with multithreaded approach
// --------------------------------------------------
void Server::connectionHandler(SOCKET clientSock){
    // note: maybe i can do isConnectionClosed here

    // main loop for the client
    while (true)
    {
        // todo: check isNewData here
        // fixme: we assume all data is recieved. this will be a problem
        memset(&_Context.recdData, 0, sizeof(_Context.recdData));

        int error = recv(clientSock, _Context.recdData.buf, DEFAULT_RECV_BUF_LEN, 0);
        // std::cout << "\n I recieved this data : " << _Context.recdData.buf << std::endl;
        if (error > 0)
        {
            std::cout << _Context.recdData.buf << "\n";

            for (auto i = _Context.clientList.begin(); i != _Context.clientList.end(); i++)
                if (i->second != clientSock)
                    send(i->second, _Context.recdData.buf,strlen(_Context.recdData.buf),0);
        }
        else if(error == 0){
            
            _Context.clientList.erase(
                std::remove_if(_Context.clientList.begin(), _Context.clientList.end(),
                [clientSock](const auto &pair){ return clientSock == pair.second; })
                ,_Context.clientList.end());

            std::cout << "Client disconnected " << "\n";
            std::cout << "Server : ";
            for (auto i = _Context.clientList.begin(); i != _Context.clientList.end(); i++)
            {
                char disconnected[] = "Server : A client disconnected";
                send(i->second, disconnected, strlen(disconnected),0);
            }
            return;
        }
        else{
            _Context.clientList.erase(
                std::remove_if(_Context.clientList.begin(),_Context.clientList.end(),
                [clientSock](const auto &pair){ return clientSock == pair.second; })
                ,_Context.clientList.end());

            std::cout << "Client recv failed  " << WSAGetLastError() << "\n";
            std::cout << "Server : ";

            for (auto i = _Context.clientList.end(); i != _Context.clientList.end(); i++)
            {
                std::string disconnected = "Server : A client disconnected";
                send(i->second, disconnected.c_str(), disconnected.size(), 0);
            }
            return;
        }
    }
}



// ----------------------------------------------
// Accepts connection with multithreaded approach
// ----------------------------------------------
void Server::acceptHandler(){
    while (true)
    {
        SOCKADDR_STORAGE clientAddress;
        int clientAddressLen = sizeof(clientAddress);
        SOCKET clientSock = accept(_Context.sock, (LPSOCKADDR)(&clientAddress), &clientAddressLen);
        if (clientSock == INVALID_SOCKET)
        {
            std::cout << "ERROR: Accept failed with error code : " << WSAGetLastError() << "\n";
            closesocket(clientSock);
            continue;
        }
        else{
            char temp[50];
            ZeroMemory(temp, 50);
            int bytesReceived = recv(clientSock, temp, sizeof(temp), 0);
            std::string clientName;
            clientName = temp; 
            // iterate over all client list pairs
            bool exist = false;
            for (auto &&i : _Context.clientList){
                // note: we don't need to check for socket. 
                // note: because we know sock number is going to be unique everytime
                // check if the username exist.
                if (i.first == clientName){
                    exist == true;
                    break;
                }
            }
            if (exist)
            {
                const char *buf = "This username is already in use. Please try different username or reconnect later.";
                send(clientSock,buf,strlen(buf),0);
                closesocket(clientSock);
                std::cout << "Client try to connect with the existing username! Connection closed.\n"; 
                continue;
            }
            else{
                // we created new thread for client.
                std::thread clientThread = std::thread(&Server::connectionHandler,this,clientSock);
                clientThread.detach();
                // todo: i used 2 structure to hold clients. one is fd_set, one is std::map
                // todo: this can be resolved with easy doubly linked list.
                _Context.clientList.push_back(std::make_pair(clientName, clientSock));

                printAddressString((LPSOCKADDR)(&clientAddress), clientAddressLen);

                std::string msg = "Server: Welcome ";
                msg = msg + clientName;
                // sends welcome message to other clients
                for (auto i = _Context.clientList.begin(); i != _Context.clientList.end(); i++)
                    send(i->second, msg.c_str(), strlen(msg.c_str()), 0);
                
                std::cout << clientName << " connected" << "\n";
                std::cout << "Server : ";
            }
        }
    }
}

// ----------------------------------------------
// Accepts connection with multithreaded approach
// ----------------------------------------------
void Server::sendHandler(){
    std::string data;
    do
    {
        std::cout << "Server : ";
        std::getline(std::cin,data);
        data = "Server: " + data;
        for (auto i = _Context.clientList.begin(); i != _Context.clientList.end(); i++)
        {
            int error = send(i->second, data.c_str(), strlen(data.c_str()),0);
            if (error == SOCKET_ERROR)
            {
                std::cout << "send to client " << i->first << " with socket " << i->second 
                << "failed with: " << WSAGetLastError() << "\n";
            }
        }
    } while (data.size() > 0);
}


bool Server::isRunning()
{
    return _bIsRunning;
}

// -------
// Cleanup
// -------
void Server::close()
{
    if (_bInitCore)
        WSACleanup();
    if (_bInitSocket)
        closesocket(_Context.sock);
}

// -------------------------------
// Prints address string of client
// -------------------------------
void Server::printAddressString(LPSOCKADDR pSockAddr, DWORD dwSockAddrLen){
    char buf[INET6_ADDRSTRLEN];
    DWORD dwBufSize = 0;
    
    memset(buf,0,sizeof(buf));
    dwBufSize = sizeof(buf);

    if (WSAAddressToString(pSockAddr,dwSockAddrLen,NULL,buf,&dwBufSize) == SOCKET_ERROR)
    {
        std::cout << "ERROR: WSAAddressToString failed with : " << WSAGetLastError() << std::endl;
        return; 
    }
    std::cout << buf << std::endl;
}

// ------------------
// Prints server info
// ------------------
void Server::printServerInfo()
{
    std::cout
            << "\n"
            << "            Server Info\n"
            << "----------------------------------\n"
            << "Interface : "  << _Context.pInterface   << "\n"
            << "Port      : "  << _Context.pPort     << "\n"
            << "----------------------------------\n";
}

// ---------------------------
// Prints how to use arguments
// ---------------------------
void Server::printHelp()
{
    std::cout 
        << "\n----------------------------------------------------\n"
        << "for interface name : -s <interface> (by default localhost)\n"
        << "for port number : -p <port> (by default 27015)\n"
        << "for help -h or -?\n"
        << "----------------------------------------------------\n";
}