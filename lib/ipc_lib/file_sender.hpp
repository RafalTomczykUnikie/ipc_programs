#pragma once

#include <string>

#include "ipc_command_sender.hpp"

class FileSender
{  
public:
    enum file_tx_err_t
    {
        FILE_SEND_OK,
        FILE_SEND_ERROR
    };

    enum file_tx_agreement_t
    {
        FILE_AGREMENT_OK,
        FILE_AGREMENT_ERROR
    };

    FileSender(IpcCommandSender *command_sender)
    : m_command_sender(command_sender)
    {

    };

    FileSender() = delete;
    FileSender(const FileSender & other) = delete;
    const FileSender &operator=(const FileSender &) = delete;

    virtual file_tx_agreement_t connectionAgrrement(std::string file_name, std::string file_extension, size_t file_size) = 0;
    virtual file_tx_err_t sendFile(void *file_ptr, int file_descriptor, std::string file_name, std::string file_extension) = 0;

protected:
    IpcCommandSender *m_command_sender = nullptr;
};