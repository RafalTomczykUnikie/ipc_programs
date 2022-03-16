#pragma once

#include <sys/socket.h>
#include <sys/un.h>
#include <cstdint>
#include <string>

#include "domain_socket_common.hpp"

class UnixDomainSocketServer
{
public:
    enum socket_server_state_t : int
    {
        CLOSED,
        OPEN,
        FAULT
    };

    enum socket_error_t : int
    {
        NO_ERROR,
        PATH_REMOVE_ERROR,
        CREATE_ERROR,
        BIND_ERROR,
        DATA_RECEIVE_ERROR,
        DATA_SEND_ERROR,
        WRONG_MSG_HEADER,
        WRONG_STATE_ERROR
    };

    UnixDomainSocketServer(const char * socket_path, int socket_type);
    ~UnixDomainSocketServer();

    socket_error_t receiveFileDesc(int *fileDescriptor = nullptr);
    socket_error_t sendMessage(uint8_t *data, uint32_t len);
    socket_error_t recvMessage(uint8_t *data, uint32_t *len);
    socket_error_t bind(void);
    socket_error_t buildAddress(void);

    socket_error_t setClientAddress(std::string client_path);
    socket_server_state_t getSocketState(void);

    int getSocketType(void);   
    
private:   
    std::string m_socket_path;
    int m_socket_type;
    int  m_sfd;
    sockaddr_un m_addr;
    sockaddr_un m_client_addr;
    socket_server_state_t m_socket_state;
    socket_error_t m_socket_error;

};