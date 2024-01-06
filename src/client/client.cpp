#include "client.h"

#include <thread>
#include <random>
#include <regex>
#include <sstream>

// todo : add assert for initialization functions

// ---------------
// init everything
// ---------------
bool Client::initEverything(int argc, char **argv){
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
bool isValidArgParameter(char *arg){
    std::basic_regex reg("[^a-zA-Z0-9]");
    // if given argument is contains special chars it will fail
    if (std::regex_search(arg,reg))
        return false;
    return true;
}

// ---------------------------------
// parse the arguments given by user
// ---------------------------------
bool Client::parseArgs(int argc, char **argv){
    //! this is a easy version of a argument parser
    //! it's only supports the following arguments:
    //! -s <server> -p <port> -u <username> -h for help
    // todo: later on implement these aswell : -d <delay> -l <sendBufLen> -r <recvBufLen>
    std::cout << "Parsing arguments...\n";

    _bParseArgs = true;

    _Context.addrFamily     =   DEFAULT_ADDR_FAMILY;
    _Context.pServer        =   DEFAULT_SERVER;
    _Context.pPort          =   DEFAULT_PORT;

    // note : this is not reliable solution
    // randomized username
    std::random_device rd;
    std::uniform_int_distribution<int> dist(1,1000);
    _Context.userName = DEFAULT_USERNAME + std::to_string(dist(rd));

    for (int i = 1; i < argc; i++){
        char firstChar = argv[i][0];
        // make sure first char starts with '-'
        if (!(firstChar == '-')){
            std::cout << "ERROR: Parsing failed! Option has to begin with '-' : " <<  argv[i] << std::endl;
            _bParseArgs = false;\
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
                _Context.pServer = argv[i];
            else{
                std::cout << "ERROR: Parsing failed! Server name needed for -s option.\n";
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
        case 'u':
            if (i + 1 >= argc){
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
bool Client::initCore(){
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
bool Client::initSocket(){
    _bInitSocket = false;

    struct addrinfo *result = nullptr, *ptr = nullptr, hints;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family     =   _Context.addrFamily;
    hints.ai_protocol   =   IPPROTO_TCP;
    hints.ai_socktype   =   SOCK_STREAM;

    // this getaddrinfo can return multiple ip adresses
    if (getaddrinfo(_Context.pServer, _Context.pPort, &hints, &result) != 0){
        std::cout << "getaddrinfo failed with Error:: " <<  WSAGetLastError() << std::endl;
        _bInitSocket = false;
        if (result)
            freeaddrinfo(result);
        return _bInitSocket;
    }

    // this will handle multiple ip adresses
    for(ptr = result; ptr != nullptr; ptr = ptr->ai_next){
        _Context.sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (_Context.sock == INVALID_SOCKET){
            std::cout << "socket failed with Error:: " <<  WSAGetLastError() << std::endl;
            std::cout << "Trying next address...\n\n";
            continue;
        }

        std::cout << "Socket created successfully with handle = " <<  _Context.sock << std::endl;
        if (connect(_Context.sock, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR){
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
void Client::run(){
    if (send(_Context.sock, _Context.userName.c_str(), strlen(_Context.userName.c_str()), 0) == SOCKET_ERROR){
        std::cout << "Sending Failed with error code : " << WSAGetLastError()  << "\n";
        return;
    }

    std::thread sendThread = std::thread(&Client::sendHandler,this);
    sendThread.detach(); // detach from current thread

    std::thread recvThread = std::thread(&Client::recvHandler,this);
    recvThread.join(); // join to current

    std::cout << "Client Shutting Down...! \n";
}

void Client::close(){
    if (_bInitCore)
        WSACleanup();
    if (_bInitSocket)
        closesocket(_Context.sock);
}

// -----------------------------------------------
// Handler for multi threaded approach to send data
// -----------------------------------------------
void Client::sendHandler(){
    // todo: add terminate flag
    do{
        std::getline(std::cin, _Context.sendBuf);
        if (_Context.sendBuf.size() > 0){
            sendMessage();
        }
        else if ( _Context.sendBuf.size() == 0)
        {
            std::cout << "Please send appopriate data!" << std::endl;
            continue;
        }
    } while (!_bPeerShutdown && !_bSocketError);
}

// -----------------------------------------------
// Handler for multi threaded approach to recv data
// -----------------------------------------------
void Client::recvHandler(){
    while (!_bSocketError && !_bPeerShutdown){
        char recvbuf[512];
        ZeroMemory(recvbuf, 512);
        auto iRecv = recv(_Context.sock, recvbuf, 512, 0);

        if (iRecv > 0){
            std::cout << '\r';
            std::cout << recvbuf << "\n";
        }
        else if (iRecv == 0) {
            std::cout << "Connection closed" << "\n";
            return;
        }
        else{
            std::cout << "recv failed: " << WSAGetLastError() << "\n";
            return;
        }
    }
}

// ------------------
// Prints server info
// ------------------
void Client::printClientInfo(){
    std::cout
            << "\n"
            << "            Client Info\n"
            << "----------------------------------\n"
            << "Server : "  << _Context.pServer   << "\n"
            << "Port   : "  << _Context.pPort     << "\n"
            // << "Delay  : "  << _Context.delay     << "\n"
            << "User   : "  << _Context.userName  << "\n"
            << "----------------------------------\n";
}

// ---------------------------
// Prints how to use arguments
// ---------------------------
void Client::printHelp(){
    std::cout
            << "\n----------------------------------------------------\n"
            << "for server name : -s <server> (by default localhost)\n"
            << "for port number : -p <port> (by default 27015)\n"
            << "for user name   : -u <username> (by default anon)\n"
            << "for help -h or -?\n"
            << "----------------------------------------------------\n";
}