#pragma once
#include <limits.h>
#include "file_receiver.hpp"

class PipeFileReceiver : public FileReceiver
{
    int m_pipe_receiver_fd = 0;
    uint8_t m_buf[PIPE_BUF];

    int RxTx(IpcCommand::ipc_command_tx_t tx, IpcCommand::ipc_command_rx_t rx_ok, IpcCommand::ipc_command_rx_t rx_nok);

public:
    PipeFileReceiver(IpcCommandReceiver *command_receiver);

    virtual file_rx_agreement_t connectionAgrrement(void);
    virtual file_rx_err_t receiveFile(const char *output_path);
};

