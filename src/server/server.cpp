#include "server.h"

#include <thread>
#include <regex>
#include <random>

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

// -------------------------
// simple argument validator
// -------------------------
bool isValidArgParameter(char *arg){
    std::regex reg("[^a-zA-Z0-9]");
    if (std::regex_search(arg,reg))
        return false;
    return true;
}
// fixme: when user selects 'h' or '?' arg. application says arg parsing failed.
// ---------------------------------
// parse the arguments given by user
// ---------------------------------
bool Server::parseArgs(int argc, char **argv){
    printf("Parsing Arguments...\n");

    _Context.addrFamily =   DEFAULT_ADDR_FAMILY;
    _Context.pInterface =   DEFAULT_SERVER;
    _Context.pPort      =   DEFAULT_PORT;

    _bParseArgs = true;

    for (int i = 1; i < argc; i++){
        char firstChar = argv[i][0];
        if (!(firstChar == '-')){
            std::cout << "ERROR: Parsing failed! Option has to begin with '-' :" << argv[i] << std::endl;
            _bParseArgs = false;
            break; 
        }
        switch (argv[i][1])
        {
        case 's':
            if (i + 1 >= argc){
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
            if (i + 1 >= argc){
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
bool Server::initCore(){
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
bool Server::initListeningSockets(){
    _bInitSocket = true;

    unsigned long nonBlocking = 1;

    SOCKET newSock;
    struct addrinfo *result = nullptr, *ptr = nullptr, hints;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family     =    _Context.addrFamily;
    hints.ai_protocol   =   IPPROTO_TCP;
    hints.ai_socktype   =   SOCK_STREAM;
    hints.ai_flags      =   AI_PASSIVE;

    if (getaddrinfo(_Context.pInterface, _Context.pPort, &hints, &result) != 0){
        printf("getaddrinfo failed. Error = %d\n", WSAGetLastError());
        _bInitSocket = false;
        if (result)
            freeaddrinfo(result);
        return _bInitSocket;
    }
    for (ptr = result; ptr != nullptr; ptr = ptr->ai_next){
        newSock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (newSock == INVALID_SOCKET){
            printf("socket failed. Error = %d\n", WSAGetLastError());
            printf("Ignoring this address and continuing with the next. \n\n");
        }

        printf("Created socket with handle = %d\n", newSock);

        if (bind(newSock, ptr->ai_addr, ptr->ai_addrlen)){
            printf("bind failed. Error = %d\n",WSAGetLastError());
            closesocket(newSock);
            continue;
        }

        printf("Socket Bound Succesfully\n");

        if (listen(newSock, MAX_CLIENTS) != 0){
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
void Server::run(){
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
    while (true){
        // todo: check isNewData here
        // fixme: we assume all data is recieved. this can be a problem

        char buf[DEFAULT_RECV_BUF_LEN];
        int error = recv(clientSock, buf, DEFAULT_RECV_BUF_LEN, 0);

        _Context.recdData.buf = buf;
        if (!parseMessage(clientSock, error)) return;
    }
}

// ----------------------------------------------
// Accepts connection with multithreaded approach
// ----------------------------------------------
void Server::acceptHandler(){
    while (true){
        SOCKADDR_STORAGE clientAddress;
        int clientAddressLen = sizeof(clientAddress);
        SOCKET clientSock = accept(_Context.sock, (LPSOCKADDR)(&clientAddress), &clientAddressLen);
        if (clientSock == INVALID_SOCKET){
            std::cout << "ERROR: Accept failed with error code : " << WSAGetLastError() << "\n";
            closesocket(clientSock);
            continue;
        }
        else{
            // get username here
            char temp[50];
            ZeroMemory(temp, 50);
            int bytesReceived = recv(clientSock, temp, sizeof(temp), 0);
            std::string clientName;
            clientName = temp; 
            // iterate over all client pairs
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
            if (_Context.clientList.size() >= MAX_CLIENTS){
                std::string buf = "Server Room is full. Pls try again later.";
                send(clientSock, buf.c_str(), buf.size(),0);
            }
            
            if (exist){
                std::string buf = "This username is already in use. Please try different username or reconnect later.";
                send(clientSock, buf.c_str(), buf.size(),0);
                closesocket(clientSock);
                std::cout << "Client try to connect with the existing username! Connection closed.\n"; 
                continue;
            }
            else{
                // we created new thread for client.
                std::thread clientThread = std::thread(&Server::connectionHandler,this,clientSock);
                clientThread.detach();

                _Context.clientList.push_back(std::make_pair(clientName, clientSock));

                // prints client info
                printAddressString((LPSOCKADDR)(&clientAddress), clientAddressLen);

                std::string msg = "Server : Welcome ";
                msg = msg + clientName;

                // sends welcome message to other clients
                for (auto i = _Context.clientList.begin(); i != _Context.clientList.end(); i++)
                    send(i->second, msg.c_str(), strlen(msg.c_str()), 0);
                
                std::cout << clientName << " connected" << "\n";
            }
        }
    }
}

// ----------------------------------------------
// Accepts connection with multithreaded approach
// ----------------------------------------------
void Server::sendHandler(){
    std::string data;
    do{
        std::getline(std::cin,data);
        data = "Server: " + data;
        for (auto i = _Context.clientList.begin(); i != _Context.clientList.end(); i++){
            int error = send(i->second, data.c_str(), strlen(data.c_str()),0);
            if (error == SOCKET_ERROR){
                std::cout << "send to client " << i->first << " with socket " << i->second 
                << "failed with: " << WSAGetLastError() << "\n";
            }
        }
    } while (data.size() > 0);
}


bool Server::isRunning(){
    return _bIsRunning;
}

// -------
// Cleanup
// -------
void Server::close(){
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
void Server::printServerInfo(){
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
void Server::printHelp(){
    std::cout 
        << "\n----------------------------------------------------\n"
        << "for interface name : -s <interface> (by default localhost)\n"
        << "for port number : -p <port> (by default 27015)\n"
        << "for help -h or -?\n"
        << "----------------------------------------------------\n";
}

bool Server::parseMessage(SOCKET sock, int error){

    std::stringstream stream(_Context.recdData.buf); // user input

    std::string command, username, message, parityBits, crcBits;
    std::getline(stream, command, ' ');
    std::getline(stream, username, ' ');
    std::getline(stream, message, '|');
    // Remove leading and trailing whitespaces
    message.erase(0, message.find_first_not_of(' ')); 
    message.erase(message.find_last_not_of(' ') + 1);
    std::getline(stream, parityBits, '|');
    parityBits.erase(0, parityBits.find_first_not_of(' ')); 
    parityBits.erase(parityBits.find_last_not_of(' ') + 1);
    std::getline(stream, crcBits);
    crcBits.erase(0, crcBits.find_first_not_of(' ')); 
    crcBits.erase(crcBits.find_last_not_of(' ') + 1);
   
    std::cout   << "\n---------  DEBUG INFO -----------\n"
                << "command : " << command  << "\n"
                << "target  : " << username << "\n"
                << "message : " << message  << "\n"
                << "parity  : " << parityBits << "\n"
                << "crcbits : " << crcBits << "\n";
                
    if(command == "/MESG" && error > 0){
        bool bTargetExist = false;
        message = username + ": " + message;
        for (auto &&i : _Context.clientList)
        {
            // send the message
            if (i.first == username){
                bTargetExist = true;

                std::cout << "I recieved this data : " << _Context.recdData.buf << "\n";
                std::random_device rd;
                std::uniform_int_distribution<int> dist(1,10);
                
                if (dist(rd) % 2)
                {
                    std::cout << "I CORRUPTED THE MESSAGE" << std::endl;
                    message += "corrupted"; // for now this is proof of concept
                    _Context.corruptedData.buf = message;
                }
                send(i.second, message.c_str(), message.size(), 0);
                break;
            }
        }
        // user not found. send to user an "user not found message"
        if (!bTargetExist){
            std::string err = "user not found !"; 
            send(sock, err.c_str(), err.size(), 0);
        }
    }
    else if (command == "/CONN"){
        std::string users = "Clients at server : ";
        // todo: if there is no user except the user himself send you are alone message. 
        for (auto &&i : _Context.clientList){
            if (i.second != sock)
                users += i.first + " ";
        }
        int error = send(sock, users.c_str(), users.size(), 0);            
        // todo:: handle error
    }
    else if(command == "/MERR"){
        for (auto &&i : _Context.clientList)
        {
            // send the message
            if (i.first == username){
                send(i.second, _Context.corruptedData.buf.c_str(), _Context.corruptedData.buf.size(), 0);
                break;
            }
        }
        // return true;
    }
    else if(command == "/GONE" || error == 0){
        std::string goodbyeMsg = "Cya later!";
        send(sock, goodbyeMsg.c_str(), goodbyeMsg.size(), 0);
        std::string user;
        for (auto i = _Context.clientList.begin(); i != _Context.clientList.end(); i++)
        {
            if (i->second == sock){
                user = i->first;
                _Context.clientList.erase(i);        
                break;
            }
        }
        
        std::cout << user << " disconnected" << "\n";
        for (auto &&i : _Context.clientList)
        {
            std::string disconnected = "Server : " + user + " disconnected";
            send(i.second, disconnected.c_str(), disconnected.size(), 0);
        }
        return false;
    }
    else{
        std::string goodbyeMsg = "Cya later!";
        send(sock, goodbyeMsg.c_str(), goodbyeMsg.size(), 0);
        std::string user;
        for (auto i = _Context.clientList.begin(); i != _Context.clientList.end(); i++)
        {
            if (i->second == sock){
                user = i->first;
                _Context.clientList.erase(i);        
                break;
            }
        }
        // todo: handle several cases. peer shutdown, conn reset etc...
        std::cout << user << " disconnected " << "\n";
        for (auto &&i : _Context.clientList)
        {
            std::string disconnected = "Server : " + user + " disconnected";
            send(i.second, disconnected.c_str(), disconnected.size(), 0);
        }
        return false;
    }
    return true;
}