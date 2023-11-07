#include "client.h"

int main(int argc, char **argv){
    Client client;
    client.initEverything(argc, argv) ? client.run() : 0;
    client.close();
    return 0;
}