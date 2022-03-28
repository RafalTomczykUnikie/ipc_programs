#include <glog/logging.h>
#include "ipc_command_receiver.hpp"


IpcCommandReceiver::command_send_resp_error_t IpcCommandReceiver::sendResponse(IpcCommandReceiver::IpcCommandRx &response)
{
    uint32_t size = sizeof(IpcCommandRx);
    uint8_t buffer[size];

     memcpy(buffer, &response, size);

    auto err = m_server->sendMessage(buffer, size);
    
    if(err != UnixDomainSocketServer::NO_ERROR)
    {
        return command_send_resp_error_t::RESPONSE_SENT_ERROR;
    }

    return command_send_resp_error_t::RESPONSE_SENT_OK;   
}


IpcCommandReceiver::command_recv_error_t IpcCommandReceiver::receiveCommand(IpcCommandReceiver::IpcCommandTx *command)
{
    uint32_t size = sizeof(IpcCommandTx);
    uint8_t buffer[size];
    
    m_current_command = *command;
    auto err = m_server->recvMessage(buffer, &size);

    memcpy(command, buffer, size);

    if(command->command == IpcCommand::ipc_command_tx_t::IPC_SEND_FILE_DESCRIPTOR)
    {
        LOG(INFO) << "received file descriptor command" << std::endl;
        auto & fd = command->file_descr.file_descriptor;
        m_server->receiveFileDesc(&fd);
        LOG(INFO) << "received file descriptor --> " << fd << std::endl;
    }

    if(err != UnixDomainSocketServer::NO_ERROR)
    {
        return command_recv_error_t::COMMAND_RECV_ERROR;
    }

    return command_recv_error_t::COMMAND_RECV_OK;
}