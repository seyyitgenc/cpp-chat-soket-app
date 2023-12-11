#include "client.h"

int main(int argc, char **argv){
    Client client;
    if (client.initEverything(argc, argv)) client.run(); else  return EXIT_FAILURE;
    client.close();
    return 0;
}