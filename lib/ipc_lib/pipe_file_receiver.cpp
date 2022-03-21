#include <glog/logging.h>

#include "pipe_file_receiver.hpp"
#include "ipc_command.hpp"


PipeFileReceiver::PipeFileReceiver(IpcCommandReceiver *command_receiver) : FileReceiver(command_receiver)
{
   
}

PipeFileReceiver::file_rx_agreement_t PipeFileReceiver::connectionAgrrement(void)
{
    auto command = IpcCommandFactory::getEmptyCommand();
    auto response = IpcCommandFactory::getEmptyResponse();
    
    auto c_rslt = m_command_receiver->receiveCommand(&command);

    if(c_rslt != IpcCommandReceiver::command_recv_error_t::COMMAND_RECV_OK || 
       command.command != IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK)
    {
        return FILE_AGR_ERROR;
    }

    response.response = IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK;

    auto r_rslt = m_command_receiver->sendResponse(response);

    if(r_rslt != IpcCommandReceiver::command_send_resp_error_t::RESPONSE_SENT_OK)
    {
        return FILE_AGR_ERROR;
    }

    LOG(INFO) << "Connection agrrement send back";

    c_rslt = m_command_receiver->receiveCommand(&command);

    if(c_rslt != IpcCommandReceiver::command_recv_error_t::COMMAND_RECV_OK || 
       command.command != IpcCommand::ipc_command_tx_t::IPC_SEND_OVER_PIPE)
    {
        return FILE_AGR_ERROR;
    }

    response.response = IpcCommand::ipc_command_rx_t::IPC_SEND_OVER_PIPE_CONFIRMED;
    r_rslt = m_command_receiver->sendResponse(response);

    if(r_rslt != IpcCommandReceiver::command_send_resp_error_t::RESPONSE_SENT_OK)
    {
        return FILE_AGR_ERROR;
    }

    LOG(INFO) << "File will be send over pipe - confirmation send back";

    c_rslt = m_command_receiver->receiveCommand(&command);

    if(c_rslt != IpcCommandReceiver::command_recv_error_t::COMMAND_RECV_OK || 
       command.command != IpcCommand::ipc_command_tx_t::IPC_SEND_FILE_METADATA)
    {
        return FILE_AGR_ERROR;
    }

    m_file_name = std::string(command.file_name);
    m_file_extension = std::string(command.file_extension);
    m_file_size = command.file_size;

    response.response = IpcCommand::ipc_command_rx_t::IPC_FILE_METADATA_OK;

    LOG(INFO) << "File metadata exchanged properly - confirmation sent back";
    LOG(INFO) << "File that will be send is -> " << m_file_name + "." + m_file_extension << " with size = " << m_file_size << " bytes";

    return file_rx_agreement_t::FILE_AGR_OK;
}
    
PipeFileReceiver::file_rx_err_t PipeFileReceiver::receiveFile(void)
{
    return file_rx_err_t::FILE_RCV_OK;
}