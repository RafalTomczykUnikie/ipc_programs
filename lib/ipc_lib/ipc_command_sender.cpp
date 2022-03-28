#include <glog/logging.h>

#include "ipc_command_sender.hpp"

IpcCommandSender::command_send_error_t IpcCommandSender::sendCommand(IpcCommandSender::IpcCommandTx &command)
{
    const auto size = sizeof(IpcCommandTx);
    uint8_t buffer[size];
    memcpy(buffer, &command, size);
    m_current_command = command;
    auto err = m_client->sendMessage(buffer, size);
    
    if(command.command == IpcCommand::IPC_SEND_FILE_DESCRIPTOR)
    {
        LOG(INFO) << "Send file descriptor command" << "\r\n";

        err = m_client->sendFileDesc(command.file_descr.file_descriptor);

        if(err != UnixDomainSocketClient::NO_ERROR)
        {
            return command_send_error_t::COMMAND_SENT_ERROR;
        }
    }

    return command_send_error_t::COMMAND_SENT_OK;
}

IpcCommandSender::command_response_error_t IpcCommandSender::receiveResponse(IpcCommandSender::IpcCommandRx *response)
{
    uint32_t size = sizeof(IpcCommandRx);
    uint8_t buffer[size];

    auto err = m_client->recvMessage(buffer, &size);

    if(err != UnixDomainSocketClient::NO_ERROR)
    {
        return command_response_error_t::RESPONSE_ERROR;
    }

    memcpy(response, buffer, size);

    return command_response_error_t::RESPONSE_OK;
}

