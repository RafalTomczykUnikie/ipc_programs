#include <glog/logging.h>

#include "pipe_file_sender.hpp"


PipeFileSender::PipeFileSender(IpcCommandSender *command_sender) : 
    FileSender(command_sender),
    m_pipe_file_descrptor{0}
{
    
}

PipeFileSender::file_tx_agreement_t PipeFileSender::connectionAgrrement(std::string file_name, std::string file_extension, size_t file_size)
{
    file_tx_agreement_t retVal = FILE_AGREMENT_OK;

    auto command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK);
    auto response = IpcCommandFactory::getEmptyResponse();

    auto c_rslt = m_command_sender->sendCommand(command);
    auto r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
       response.response != IpcCommand::IPC_CONNECTION_OK)
    {
        retVal = FILE_AGREMENT_ERROR;
        return retVal;
    }

    LOG(INFO) << "Connection agrrement ok";

    command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_SEND_OVER_PIPE);
    response = IpcCommandFactory::getEmptyResponse();

    c_rslt = m_command_sender->sendCommand(command);
    r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
        response.response != IpcCommand::ipc_command_rx_t::IPC_SEND_OVER_PIPE_CONFIRMED)
    {
        retVal = FILE_AGREMENT_ERROR;
        return retVal;
    }

    LOG(INFO) << "File will be send over pipe";

    command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_SEND_FILE_METADATA, file_name, file_extension, file_size);
    response = IpcCommandFactory::getEmptyResponse();

    c_rslt = m_command_sender->sendCommand(command);
    r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
       response.response != IpcCommand::ipc_command_rx_t::IPC_FILE_METADATA_OK)
    {
        retVal = FILE_AGREMENT_ERROR;
        return retVal;
    }

    LOG(INFO) << "File metadata exchanged properly";

    return retVal;
}

PipeFileSender::file_tx_err_t PipeFileSender::sendFile(void *file_ptr, int file_descriptor, std::string file_name, std::string file_extension)
{
    return FILE_SEND_OK;
}