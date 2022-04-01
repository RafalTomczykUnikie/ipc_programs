#pragma once
#include <limits.h>
#include <mqueue.h>
#include "file_receiver.hpp"

class MessageFileReceiver : public FileReceiver
{
    static constexpr auto MAX_BUF_SIZE = 8096u;
    
    int m_maximum_data_size = 0;
    int m_maximum_queue_size = 0;
    
    mqd_t m_queue_file_descrptors = {0};
    std::string m_queue_name;
    mq_attr m_attrs;
    uint8_t m_buffer[MAX_BUF_SIZE];

    int RxTx(IpcCommand::ipc_command_tx_t tx, IpcCommand::ipc_command_rx_t rx_ok, IpcCommand::ipc_command_rx_t rx_nok);

public:
    MessageFileReceiver(IpcCommandReceiver *command_receiver, std::string queue_name);

    virtual file_rx_agreement_t connectionAgrrement(void);
    virtual file_rx_err_t receiveFile(const char *output_path);
};