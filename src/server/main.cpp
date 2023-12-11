#include "server.h"

int main(int argc, char **argv){
    Server sv;
    if (sv.initEverything(argc, argv)) sv.run(); else EXIT_FAILURE;
    sv.close();
    return 0;
}