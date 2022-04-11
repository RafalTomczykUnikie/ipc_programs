#include <glog/logging.h>
#include <fcntl.h>
#include <sys/time.h>
#include <poll.h>

#include "message_file_receiver.hpp"
#include "ipc_command.hpp"


MessageFileReceiver::MessageFileReceiver(IpcCommandReceiver *command_receiver, std::string queue_name) : 
    FileReceiver(command_receiver), 
    m_queue_name(queue_name)
{
    constexpr auto flags = O_RDONLY;
    m_queue_file_descrptors = mq_open(m_queue_name.data(), flags);
    if(m_queue_file_descrptors < 0)
    {
        throw std::runtime_error("Problem with opening a new message queue!");
    }

    auto result = mq_getattr(m_queue_file_descrptors, &m_attrs);
    if(result != 0)
    {
        throw std::runtime_error("Problem wit getting queue attributes!");
    }

    m_maximum_data_size = m_attrs.mq_msgsize;
    m_maximum_queue_size = m_attrs.mq_maxmsg;
}

MessageFileReceiver::~MessageFileReceiver()
{
    mq_close(m_queue_file_descrptors);
}

MessageFileReceiver::file_rx_agreement_t MessageFileReceiver::connectionAgrrement(void)
{
    auto res = RxTx(IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK, 
                    IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK,
                    IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR);

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    LOG(INFO) << "Connection agrrement send back";

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_SEND_OVER_MESSAGE, 
         IpcCommand::ipc_command_rx_t::IPC_SEND_OVER_MESSAGE_CONFIRMED,
         IpcCommand::ipc_command_rx_t::IPC_DIFFERENT_IPC_METHOD);

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    LOG(INFO) << "File will be send over message - confirmation send back";

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_SEND_FILE_METADATA, 
         IpcCommand::ipc_command_rx_t::IPC_FILE_METADATA_OK,
         IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR);

    std::string full_file_name = m_file_name; 

    if(m_file_extension != "")
    {
        m_file_extension += "." + m_file_extension;
    }

    LOG(INFO) << "File metadata exchanged properly - confirmation sent back";
    LOG(INFO) << "File that will be send is -> " << full_file_name << " with size = " << m_file_size << " bytes";

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK, 
         IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK,
         IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR);

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;
    
    return file_rx_agreement_t::FILE_AGR_OK;
}

int MessageFileReceiver::RxTx(IpcCommand::ipc_command_tx_t tx, IpcCommand::ipc_command_rx_t rx_ok, IpcCommand::ipc_command_rx_t rx_nok)
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

    
MessageFileReceiver::file_rx_err_t MessageFileReceiver::receiveFile(const char *output_path)
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
        timespec t;
        clock_gettime(CLOCK_REALTIME, &t);
        t.tv_sec += 10;
        auto msg_prio = 0u;
        auto s = mq_timedreceive(m_queue_file_descrptors, (char*)m_buffer, m_maximum_data_size, &msg_prio, &t);

        auto r = int(0);
        if(s > 0)
        {
            r = write(file_descriptor, m_buffer, s);
        }
                
        if(r == -1)
        {
            LOG(ERROR) << "Error in writing to a file. Aborting!";
            close(file_descriptor);
            return FILE_RCV_ERROR;
        }

        if(s < m_maximum_data_size)
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