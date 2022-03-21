#pragma once

#include <sys/socket.h>
#include <sys/un.h>
#include <cstdint>
#include <string>

#include "domain_socket_common.hpp"

class UnixDomainSocketClient
{

public:

    enum socket_client_state_t : int
    {
        DISCONNECTED,
        CONNECTED,
        FAULT
    };

    enum socket_client_error_t : int
    {
        NO_ERROR,
        WRONG_STATE_ERROR,
        DATA_SEND_ERROR,
        DATA_RECEIVE_ERROR,
        CREATE_ERROR,
        CONNECTION_ERROR,
        WRONG_MSG_HEADER,
        BIND_ERROR
    };

    enum socket_bind_t : int
    {
        BIND_TO_CLIENT,
        BIND_TO_SERVER
    };

    UnixDomainSocketClient(const char * socket_path, int socket_type);
    ~UnixDomainSocketClient();

    socket_client_error_t sendFileDesc(int fileDescriptor = 0);
    socket_client_error_t recvMessage(uint8_t *data, uint32_t *len);
    socket_client_error_t sendMessage(uint8_t *data, uint32_t len);
    socket_client_error_t buildAddress(void);
    socket_client_error_t connect(void);
    socket_client_error_t bind(socket_bind_t bind_sel = BIND_TO_CLIENT);
    socket_client_error_t setServerAddress(std::string server_path);
    socket_client_state_t getState(void);
    void close(void);

private:
    std::string m_socket_path;
    sockaddr_un m_addr;
    sockaddr_un m_server_address;
    int m_socket_type;
    socket_client_state_t m_socket_state;
    socket_client_error_t m_socket_error;
    int m_sfd;
    int m_sfd_client;

};