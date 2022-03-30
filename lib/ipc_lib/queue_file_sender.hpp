#pragma once
#include <string>
#include <limits.h>
#include <mqueue.h>
#include "file_sender.hpp"

class QueueFileSender : public FileSender
{
private:
    static constexpr auto MAX_BUF_SIZE = 8096u;
    static constexpr auto MAX_QUEUE_SIZE = 10u;
    
    const int m_maximum_data_size = MAX_BUF_SIZE;
    const int m_maximum_queue_size = MAX_QUEUE_SIZE;
    
    mqd_t m_queue_file_descrptors = {0};
    std::string m_queue_name;
    mq_attr m_attrs;
    uint8_t m_buffer[MAX_BUF_SIZE];

public:
    QueueFileSender(IpcCommandSender *command_sender, std::string queue_name);
    ~QueueFileSender();

    virtual file_tx_agreement_t connectionAgrrement(std::string file_name, std::string file_extension, size_t file_size);
    virtual file_tx_err_t sendFile(const char * file_path);
};