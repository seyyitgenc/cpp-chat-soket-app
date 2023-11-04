#include "client.h"

int main(int argc, char** argv){
    Client client;
    client.init(argc, argv);
    client.run();
    client.close();

    return 0;
}