#pragma once

#include "globals.h"

// ? should we make it singleton? 
// note: i think we should

// !!!! MAYBE THERE WILL BE A BUG IN THE FUTURE WHILE CLOSING THE APP!!!!
class Client
{
public:
    Client() {};
    ~Client() = default;
    bool init(int argc, char** argv){
        // init winsock
        if (argc != 2)
        {
            printf("Error: invalid arguments, provide it like this\n");
            printf("usage: server-name, server_ip\n");
            return false;
        }
        
        m_iResult = WSAStartup(MAKEWORD(2,2),&m_wsaData);
        if (m_iResult != 0)
        {
            printf("WSAStartup failes: %d\n",m_iResult);
            return false;
        }

        ZeroMemory(&m_hints, sizeof(m_hints));
        m_hints.ai_family   = AF_INET; // for IPv4
        m_hints.ai_socktype = SOCK_STREAM;
        m_hints.ai_protocol = IPPROTO_TCP;

        // note: while we run the client, we need to send the ip address as an argument.
        //! we need to run this app like this --> ./client server_name ip_address
        m_iResult = getaddrinfo(argv[1], DEFAULT_PORT, &m_hints, &m_result);

        if(m_iResult != 0){
            printf("getaddrinfo failed: %d\n",m_iResult);
            return false;
        }
        
        // attemp to connect address returned by getaddrinfo
        m_connectSocket = socket(m_result->ai_family, m_result->ai_socktype, m_result->ai_protocol);
        if (m_connectSocket == INVALID_SOCKET)
        {
            printf("Error at socket(): %d\n", WSAGetLastError());
            return false;     
        }
        
        freeaddrinfo(m_result);

        m_iResult = connect(m_connectSocket, m_result->ai_addr, (int)m_result->ai_addrlen);
        if (m_iResult != 0)
        {
            printf("Unable to connect to server!\n");
            return false;
        }

        return true;
    }
    bool run(){
        while (1)
        {
            printf("Enter message: ");
            fgets(m_Sendbuf, DEFAULT_BUFLEN, stdin);
            if (send(m_connectSocket, m_Sendbuf, (int)strlen(m_Sendbuf), 0) == SOCKET_ERROR)
            {
                printf("send failed: %d\n", WSAGetLastError());
                return false;
            }
            memset(m_Recvbuf, '\0', DEFAULT_BUFLEN);
            if(recv(m_connectSocket, m_Recvbuf, DEFAULT_BUFLEN, 0) == SOCKET_ERROR){
                printf("recv failed: %d\n", WSAGetLastError());
                return false;
            }
            puts(m_Recvbuf);;
        }
        
    }
    void close(){
        freeaddrinfo(m_result);
        freeaddrinfo(m_ptr);
        closesocket(m_connectSocket);
        WSACleanup();
    }
private:
    WSADATA m_wsaData;
    SOCKET m_connectSocket = INVALID_SOCKET;
    int m_iResult;

    // ! impelement a feature to get the multiple ip adresses from the getaddrinfo
    // ! we can do this by using m_ptr
    struct addrinfo *m_result = NULL, *m_ptr = NULL, m_hints;
    // struct sockaddr_in m_sockaddr_in;d
    char m_Sendbuf[DEFAULT_BUFLEN];
    char m_Recvbuf[DEFAULT_BUFLEN];

};