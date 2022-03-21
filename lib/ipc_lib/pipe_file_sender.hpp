#pragma once

#include "file_sender.hpp"


class PipeFileSender : public FileSender
{
private:
    int m_pipe_file_descrptor; 

public:
    PipeFileSender(IpcCommandSender *command_sender);

    virtual file_tx_agreement_t connectionAgrrement(std::string file_name, std::string file_extension, size_t file_size);
    virtual file_tx_err_t sendFile(void *file_ptr, int file_descriptor, std::string file_name, std::string file_extension);
};