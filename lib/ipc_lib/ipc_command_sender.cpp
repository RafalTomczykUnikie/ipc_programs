#include "ipc_command_sender.hpp"
#include "ipc_command.hpp"

IpcCommandSender::command_send_error_t sendCommand(IpcCommandSender::IpcCommandTx &command)
{
    if(command.command != IpcCommand::IPC_SEND_FILE_DESCRIPTOR)
    {
        m_client->
    }
}

IpcCommandSender::command_response_error_t receiveResponse(IpcCommandSender::IpcCommandRx *response)
{

}

