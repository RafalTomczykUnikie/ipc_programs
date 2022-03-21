#pragma once

#include "file_receiver.hpp"

class PipeFileReceiver : public FileReceiver
{
public:

    PipeFileReceiver(IpcCommandReceiver *command_receiver);

    virtual file_rx_agreement_t connectionAgrrement(void);
    virtual file_rx_err_t receiveFile(void);
};

