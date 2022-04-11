#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <glog/logging.h>
#include "pipe_file_sender.hpp"

PipeFileSender::PipeFileSender(IpcCommandSender *command_sender, std::string pipe_name) : 
    FileSender(command_sender),
    m_pipe_name(pipe_name)
{
    auto res = mkfifo(m_pipe_name.c_str(), 0777);

    m_pipe_receiver_fd = open(m_pipe_name.c_str(), O_WRONLY);
    if(m_pipe_receiver_fd < 0)
    {
        LOG(ERROR) << "Cannot create pipe!" << "\r\n";
        throw std::runtime_error("Cannot create a pipe");
    }
}

PipeFileSender::~PipeFileSender()
{
    close(m_pipe_receiver_fd);
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
    LOG(INFO) << "Checking connection again...";

    command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK);
    response = IpcCommandFactory::getEmptyResponse();

    c_rslt = m_command_sender->sendCommand(command);
    r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
        response.response != IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK)
    {
        retVal = FILE_AGREMENT_ERROR;
        return retVal;
    }

    LOG(INFO) << "Connection checked properly!";

    return retVal;
}

PipeFileSender::file_tx_err_t PipeFileSender::sendFile(const char * file_path)
{
    auto command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_START_FILE_TRANSFER);
    auto response = IpcCommandFactory::getEmptyResponse();

    auto c_rslt = m_command_sender->sendCommand(command);
    auto r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
       response.response != IpcCommand::ipc_command_rx_t::IPC_START_FILE_TRANSFER_CONFIRMED)
    {
        return FILE_SEND_ERROR;
    }  

    auto sendError = [&](void){
        command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_DATA_SEND_ERROR);
        response = IpcCommandFactory::getEmptyResponse();
        c_rslt = m_command_sender->sendCommand(command);
        r_rslt = m_command_sender->receiveResponse(&response);

        if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
        r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
        response.response != IpcCommand::ipc_command_rx_t::IPC_ALL_DATA_RECEIVED_ERROR)
        {
            return FILE_SEND_ERROR;
        } 
        return FILE_SEND_ERROR;
    };

    LOG(INFO) << "Starting file transfer...";

    auto file_descriptor = open(file_path, O_RDONLY);

    if(file_descriptor == -1)
    {
        LOG(ERROR) << "Cannot open file specified";
        return sendError();
    }

    auto s = 0;
    auto idx = 0ul;
    
    while(1)
    {   
        s = read(file_descriptor, m_buf, PIPE_BUF);
        if(s == 0)
        {
            LOG(INFO) << "End of file is reached, finishing...";
            break;
        }

        auto r = write(m_pipe_receiver_fd, m_buf, s);
        
        if(r == -1)
        {   
            continue;
        }
    }

    close(file_descriptor);

    LOG(INFO) << "End of file is reached, finishing...";

    command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_ALL_DATA_SENT);
    response = IpcCommandFactory::getEmptyResponse();
    c_rslt = m_command_sender->sendCommand(command);
    r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
       response.response != IpcCommand::ipc_command_rx_t::IPC_ALL_DATA_RECEIVED_OK)
    {
        return FILE_SEND_ERROR;
    } 

    LOG(INFO) << "File transfer is successful";

    return FILE_SEND_OK;
}