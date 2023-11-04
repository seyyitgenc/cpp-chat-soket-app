#pragma once
    
#include "globals.h"

class Server
{
private:
    Server() = default;
public:
    ~Server() = default;
    // Singleton
    static Server *getInstance()
    {
        if (m_instance == nullptr)
        {
            printf("Server instance created\n");
            m_instance = new Server();
        }
        return m_instance;
    }

    bool init(){
        m_iResult = WSAStartup(MAKEWORD(2,2),&m_WsaData);
        if (m_iResult != 0)
        {
            printf("WSAStartup failes: %d\n", m_iResult);
            return false;
        }

        ZeroMemory(&m_Hints, sizeof(m_Hints));
        m_Hints.ai_family   = AF_INET;
        m_Hints.ai_socktype = SOCK_STREAM;
        m_Hints.ai_protocol = IPPROTO_TCP;
        m_Hints.ai_flags    = AI_PASSIVE;

        // resolve the local address port to be used by the server  
        m_iResult = getaddrinfo(NULL, DEFAULT_PORT, &m_Hints, &m_Result);
        if (m_iResult != 0)
        {
            printf("getaddrinfo failed: %d\n", m_iResult);
            return false;
        }

        // create socket
        m_ListenSocket = INVALID_SOCKET;
        m_ListenSocket = socket(m_Result->ai_family, m_Result->ai_socktype, m_Result->ai_protocol);
        if (m_ListenSocket == INVALID_SOCKET)
        {
            printf("Error at socket(): %d\n", WSAGetLastError());
            return false;
        }

        // setup the TCP listening socket
        m_iResult = bind(m_ListenSocket, m_Result->ai_addr, (int)m_Result->ai_addrlen);
        if (m_iResult == SOCKET_ERROR){
            printf("bind failed with error: %d\n",WSAGetLastError());
            return false;
        }

        freeaddrinfo(m_Result);
        
        if (listen(m_ListenSocket, SOMAXCONN) == SOCKET_ERROR){
            printf("Listen failed with error: %d\n",WSAGetLastError());
            return false;
        }
        
        return true;
    };
    
    bool run(){
        m_ClientSocket = accept(m_ListenSocket, NULL, NULL);
        if (m_ClientSocket == INVALID_SOCKET){
            printf("accept failed: %d\n", WSAGetLastError());
            return false;
        }
        while (1)
        {
            printf("Waiting for data\n");
            fflush(stdout);
            memset(m_Recvbuf, '\0', DEFAULT_BUFLEN);
            if(recv(m_ClientSocket, m_Recvbuf, DEFAULT_BUFLEN, 0) == SOCKET_ERROR){
                printf("recv failed: %d\n", WSAGetLastError());
                return false;
            }
            printf("Recieved packet from %s:%d\n", inet_ntoa(m_ClientAddr.sin_addr), ntohs(m_ClientAddr.sin_port));
            printf("Data: %s\n", m_Recvbuf);

            if(send(m_ClientSocket, m_Recvbuf, DEFAULT_BUFLEN, 0) == SOCKET_ERROR){
                printf("send failed: %d\n", WSAGetLastError());
                return false;
            }
        }
    };
    void close(){
        closesocket(m_ClientSocket);
        closesocket(m_ListenSocket);
        WSACleanup();
    };
private:
    static Server *m_instance;

private:
    // bool wsaStarted = false;

    WSADATA m_WsaData;
    SOCKET m_ListenSocket;
    SOCKET m_ClientSocket;
    struct sockaddr_in m_ClientAddr;
    struct addrinfo *m_Result;
    struct addrinfo m_Hints;
    
    char m_Recvbuf[DEFAULT_BUFLEN];
    char m_Sendbuf[DEFAULT_BUFLEN];
    int m_iResult;
};

Server *Server::m_instance = nullptr;