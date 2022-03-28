#pragma once

#include "ipc_command.hpp"
#include "domain_socket_server.hpp"

class IpcCommandReceiver : public IpcCommand
{


public:

    IpcCommandReceiver(UnixDomainSocketServer *server) : m_server(server)
    {

    };

    enum command_recv_error_t : int
    {
        COMMAND_RECV_OK,
        COMMAND_RECV_ERROR
    };

    enum command_send_resp_error_t : int
    {
        RESPONSE_SENT_OK,
        RESPONSE_SENT_ERROR
    };

    command_send_resp_error_t sendResponse(IpcCommandRx &response); 
    command_recv_error_t receiveCommand(IpcCommandTx *command);
    command_recv_error_t receiveCommandFileDesc(IpcCommandTx *command);
    

private:
    UnixDomainSocketServer *m_server = nullptr;
    IpcCommandTx m_current_command;
};