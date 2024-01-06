#include "client.h"
#include <sstream>
#include <bitset>

void Client::parseMessage(ClientType type)
{
    if (type == SENDER)
    {
        std::stringstream stream(_Context.sendBuf); // user input
        std::string word;
        stream >> word; // get the argument
        char first_char = word[0];
        if (first_char == '/')
        {
            if(word == "/MESG"){
                stream >> word; // send target. A.K.A user

                std::string msg;
                // get the message
                while (stream >> word){
                    if (!msg.empty()) {
                        msg += " ";
                    }
                    msg += word;
                }
                
                generateErrorBits(msg);

                int error = send(_Context.sock, _Context.sendBuf.c_str(), _Context.sendBuf.size(), 0);
            }
            else if (word == "/CONN"){
                // todo:: handle error
                int error = send(_Context.sock, _Context.sendBuf.c_str(),_Context.sendBuf.size(), 0);
                return;
            }
            else if(word == "/MERR"){
                // todo:: handle error
                int error = send(_Context.sock, _Context.sendBuf.c_str(),_Context.sendBuf.size(), 0);
                return;
            }
            else if(word == "/GONE"){
                // todo:: handle error
                int error = send(_Context.sock, _Context.sendBuf.c_str(),_Context.sendBuf.size(), 0);
                _bPeerShutdown = true;
                shutdown(_Context.sock, SD_BOTH);
                return;
            }
            else{
                // todo: add print function print all functionalities
                std::cout   << "ERROR: Undefined argument pls provide valid argument."
                            << "Provided argument is : " << word << "\n";
            }
        }
        else
            std::cout << "ERROR: please to provide an argument use this literal before argument -> '/'. \n";
    }
    else if(type == RECIEVER){
    }
    else{
        std::cout << "ERROR: given client type is not valid!" << std::endl;
    }
}

void Client::sendMessage()
{
    parseMessage(SENDER);
}

void Client::recvMessage()
{
    char buf[DEFAULT_RECV_BUF_LEN];
    memset(buf, 0, DEFAULT_RECV_BUF_LEN);
    int error = recv(_Context.sock, buf, strlen(buf), 0);
    // todo: handle error

    _Context.recvBuf.clear(); // in case
    _Context.recvBuf = buf;

    parseMessage(RECIEVER);
}

void Client::generateErrorBits(std::string input)
{
    std::string crcBits = cyclicRedundancyCheck(input);
    std::string spBits = simpleParityCheck(input);
    _Context.sendBuf = _Context.sendBuf + " | " + spBits + " | " + crcBits;
}

std::string Client::simpleParityCheck(std::string msg)
{
    std::string result;
    for (char c : msg)
    {
        unsigned int bits = std::bitset<8>(c).count();
        result += (bits % 2 == 0) ? '0' : '1';
    }
    return result;
}

std::string Client::cyclicRedundancyCheck(std::string msg)
{ 
    const int g = 0x1021; // Predefined generator polynomial
    std::string result;
    for (char c : msg) {
        int crc = 0;
        for (int i = 7; i >= 0; i--) {
            int bit = (c >> i) & 1;
            crc = (crc << 1) | bit;
            if (crc & 0x10000) {
                crc ^= g;
            }
        }
        result += std::to_string(crc & 0xffff); // Append CRC code
    }
    return result;
}