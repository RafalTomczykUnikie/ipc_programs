#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#include <limits.h>
#include <glog/logging.h>
#include "message_file_sender.hpp"

MessageFileSender::MessageFileSender(IpcCommandSender *command_sender, std::string queue_name) :
    FileSender(command_sender),
    m_queue_name(queue_name)
{
    m_attrs.mq_curmsgs = 0;
    m_attrs.mq_flags = 0;
    m_attrs.mq_msgsize = m_maximum_data_size;
    m_attrs.mq_maxmsg = m_maximum_queue_size;

    const auto flags = O_RDWR | O_CREAT;
    
    const mode_t mode = 0666;
    
    m_queue_file_descrptors = mq_open(m_queue_name.data(), flags, mode, &m_attrs);
    
    if(m_queue_file_descrptors < 0)
    {
        LOG(ERROR) << "m_queue_file_descrptors = " << m_queue_file_descrptors;
        throw std::runtime_error("Problem with creating a new message queue!");
    }

}

MessageFileSender::~MessageFileSender()
{
    mq_close(m_queue_file_descrptors);
}

MessageFileSender::file_tx_agreement_t MessageFileSender::connectionAgrrement(std::string file_name, std::string file_extension, size_t file_size)
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

    command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_SEND_OVER_MESSAGE);
    response = IpcCommandFactory::getEmptyResponse();

    c_rslt = m_command_sender->sendCommand(command);
    r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
        response.response != IpcCommand::ipc_command_rx_t::IPC_SEND_OVER_MESSAGE_CONFIRMED)
    {
        retVal = FILE_AGREMENT_ERROR;
        return retVal;
    }

    LOG(INFO) << "File will be send over message queue";

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

MessageFileSender::file_tx_err_t MessageFileSender::sendFile(const char * file_path)
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

        return FILE_SEND_ERROR;
    };

    LOG(INFO) << "Starting file transfer...";

    auto file_descriptor = open(file_path, O_RDONLY);

    if(file_descriptor == -1)
    {
        LOG(ERROR) << "Cannot open file specified";
        return sendError();
    }

    auto s = 0u;

    while(1)
    {
        s = read(file_descriptor, m_buffer, MAX_BUF_SIZE);
       
        auto r = mq_send(m_queue_file_descrptors, (char *)m_buffer, s, 0);
        if(r == -1)
        {
            LOG(ERROR) << "Error in sending a message. Aborting!";
            close(file_descriptor);
            return sendError();
        }

        if(s < MAX_BUF_SIZE)
        {
            LOG(INFO) << "End of file is reached, finishing...";
            break;
        }
    }

    close(file_descriptor);

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