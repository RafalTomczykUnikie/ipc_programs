#pragma once

#include <string>

#include "ipc_command_receiver.hpp"

class FileReceiver
{  
    virtual int RxTx(IpcCommand::ipc_command_tx_t tx, IpcCommand::ipc_command_rx_t rx_ok, IpcCommand::ipc_command_rx_t rx_nok) = 0;

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
    virtual file_rx_err_t receiveFile(const char *output_path) = 0;
    

protected:
    IpcCommandReceiver *m_command_receiver = nullptr;
    std::string m_file_name = "";
    std::string m_file_extension = "";
    size_t m_file_size = 0;
};