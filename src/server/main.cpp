#include "server.h"


int main(int argc, char **argv){
    Server *server = Server::getInstance();
    
    if (!server->init()) server->close();
    if (!server->run())  server->close();
    server->close();

    return 0;
}