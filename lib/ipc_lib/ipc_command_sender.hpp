#pragma once

#include "ipc_command.hpp"
#include "domain_socket_client.hpp"

class IpcCommandSender : public IpcCommand
{


public:

    IpcCommandSender(UnixDomainSocketClient *client) : m_client(client)
    {

    };

    enum command_send_error_t : int
    {
        COMMAND_SENT_OK,
        COMMAND_SENT_ERROR
    };

    enum command_response_error_t : int
    {
        RESPONSE_OK,
        RESPONSE_ERROR
    };

    command_send_error_t sendCommand(IpcCommandTx &command); 
    command_response_error_t receiveResponse(IpcCommandRx *response);

    private:
    UnixDomainSocketClient *m_client = nullptr;
    IpcCommandTx m_current_command;
};