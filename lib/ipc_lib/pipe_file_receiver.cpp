#include <glog/logging.h>
#include <fcntl.h>
#include <sys/time.h>
#include <poll.h>

#include "pipe_file_receiver.hpp"
#include "ipc_command.hpp"


PipeFileReceiver::PipeFileReceiver(IpcCommandReceiver *command_receiver) : FileReceiver(command_receiver)
{
   
}

PipeFileReceiver::file_rx_agreement_t PipeFileReceiver::connectionAgrrement(void)
{
    auto res = RxTx(IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK, 
                    IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK,
                    IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR);

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    LOG(INFO) << "Connection agrrement send back";

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_SEND_OVER_PIPE, 
         IpcCommand::ipc_command_rx_t::IPC_SEND_OVER_PIPE_CONFIRMED,
         IpcCommand::ipc_command_rx_t::IPC_DIFFERENT_IPC_METHOD);

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    LOG(INFO) << "File will be send over pipe - confirmation send back";

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_SEND_FILE_METADATA, 
         IpcCommand::ipc_command_rx_t::IPC_FILE_METADATA_OK,
         IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR);

    LOG(INFO) << "File metadata exchanged properly - confirmation sent back";
    LOG(INFO) << "File that will be send is -> " << m_file_name + "." + m_file_extension << " with size = " << m_file_size << " bytes";

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_SEND_FILE_DESCRIPTOR, 
         IpcCommand::ipc_command_rx_t::IPC_FILE_DESCRIPTOR_OK,
         IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR);

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    LOG(INFO) << "Pipe file descriptor received properly! -> fd = " << m_pipe_receiver_fd;

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK, 
         IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK,
         IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR);

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;
    
    return file_rx_agreement_t::FILE_AGR_OK;
}

int PipeFileReceiver::RxTx(IpcCommand::ipc_command_tx_t tx, IpcCommand::ipc_command_rx_t rx_ok, IpcCommand::ipc_command_rx_t rx_nok)
{
    auto command = IpcCommandFactory::getEmptyCommand();
    auto response = IpcCommandFactory::getEmptyResponse();
    
    auto c_rslt = m_command_receiver->receiveCommand(&command);

    if(c_rslt != IpcCommandReceiver::command_recv_error_t::COMMAND_RECV_OK)
    {
        return -1;
    }

    if(command.command != tx)
    {
        response.response = rx_nok;
    }
    else
    {
        if(command.command == IpcCommand::ipc_command_tx_t::IPC_SEND_FILE_METADATA)
        {
            m_file_name = std::string(command.file_name);
            m_file_extension = std::string(command.file_extension);
            m_file_size = command.file_descr.file_size;
        }
        else if(command.command == IpcCommand::ipc_command_tx_t::IPC_SEND_FILE_DESCRIPTOR)
        {
            m_pipe_receiver_fd = command.file_descr.file_descriptor;
        }

        response.response = rx_ok;
    }

    auto r_rslt = m_command_receiver->sendResponse(response);

    if(r_rslt != IpcCommandReceiver::command_send_resp_error_t::RESPONSE_SENT_OK || 
       response.response == rx_nok)
    {
        return -1;
    }

    return 0;
}

    
PipeFileReceiver::file_rx_err_t PipeFileReceiver::receiveFile(const char *output_path)
{
    std::string out_path(output_path);

    auto res = RxTx(IpcCommand::ipc_command_tx_t::IPC_START_FILE_TRANSFER, 
                    IpcCommand::ipc_command_rx_t::IPC_START_FILE_TRANSFER_CONFIRMED,
                    IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR);

    if(res == -1)
    {
        return FILE_RCV_ERROR;
    }

    LOG(INFO) << "Starting file receiving...";

    auto file_descriptor = open(out_path.data(), O_CREAT | O_WRONLY, 0666);
    if(file_descriptor == -1)
    {
        LOG(ERROR) << "Cannot write to specified path!";
    }

    while(1)
    {
        auto s = 0;

        pollfd pfds[1];
        pfds[0].fd = m_pipe_receiver_fd;
        pfds[0].events = POLLIN;

        auto retpoll = poll(pfds, 1, 10000);

        if (retpoll > 0) 
        {
            if (pfds[0].revents & POLLIN) 
            {
                s = read(m_pipe_receiver_fd, m_buf, PIPE_BUF);
            }
        }
        else if (retpoll == 0)
        {
            LOG(INFO) << "pool timeout, finishing...";
            break;
        }
        else
        {
            LOG(ERROR) << "pool failed";
            close(file_descriptor);
            return FILE_RCV_ERROR;
        }
                
        auto r = write(file_descriptor, m_buf, s);
        
        if(r == -1)
        {
            LOG(ERROR) << "Error in writing to a file. Aborting!";
            close(file_descriptor);
            return FILE_RCV_ERROR;
        }

        if(s < PIPE_BUF)
        {
            LOG(INFO) << "End of file is reached, finishing...";
            break;
        }
    }
    
    close(file_descriptor);

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_ALL_DATA_SENT, 
               IpcCommand::ipc_command_rx_t::IPC_ALL_DATA_RECEIVED_OK,
               IpcCommand::ipc_command_rx_t::IPC_ALL_DATA_RECEIVED_ERROR);
    
    if(res == -1)
    {
        return FILE_RCV_ERROR;
    }

    LOG(INFO) << "File transfer is successful";

    return file_rx_err_t::FILE_RCV_OK;
}