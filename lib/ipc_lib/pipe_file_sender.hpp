#pragma once
#include <limits.h>

#include "file_sender.hpp"


class PipeFileSender : public FileSender
{
private:
    int m_pipe_file_descrptors[2] = {0};
    uint8_t m_buf[PIPE_BUF];

public:
    PipeFileSender(IpcCommandSender *command_sender);

    virtual file_tx_agreement_t connectionAgrrement(std::string file_name, std::string file_extension, size_t file_size);
    virtual file_tx_err_t sendFile(const char * file_path);
};