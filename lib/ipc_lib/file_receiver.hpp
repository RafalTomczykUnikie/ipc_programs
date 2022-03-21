#pragma once

#include <string>

#include "ipc_command_receiver.hpp"

class FileReceiver
{  
public:
    enum file_rx_err_t
    {
        FILE_RCV_OK,
        FILE_RCV_ERROR
    };

    enum file_rx_agreement_t
    {
        FILE_AGR_OK,
        FILE_AGR_ERROR
    };

    FileReceiver(IpcCommandReceiver *command_receiver) : m_command_receiver(command_receiver)
    {

    };

    FileReceiver() = delete;
    FileReceiver(const FileReceiver & other) = delete;
    const FileReceiver &operator=(const FileReceiver &) = delete;

    virtual file_rx_agreement_t connectionAgrrement(void) = 0;
    virtual file_rx_err_t receiveFile(void) = 0;

protected:
    IpcCommandReceiver *m_command_receiver = nullptr;
    std::string m_file_name = "";
    std::string m_file_extension = "";
    size_t m_file_size = 0;
};