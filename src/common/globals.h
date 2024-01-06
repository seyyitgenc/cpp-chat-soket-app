#pragma once

#ifdef _WIN64
#include <winsock2.h>
#include <ws2tcpip.h>

#elif defined(__linux__)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h> 
#include <sys/time.h>
typedef unsigned int SOCKET;
typedef struct sockaddr		*LPSOCKADDR;
typedef unsigned long DWORD;
typedef unsigned char BYTE;
#endif

#include <iostream>

#define DEFAULT_ADDR_FAMILY     AF_INET
#define DEFAULT_SERVER          "localhost"
#define DEFAULT_PORT            "27015"