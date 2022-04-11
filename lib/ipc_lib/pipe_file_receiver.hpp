#pragma once
#include <limits.h>
#include <gtest/gtest_prod.h>
#include "file_receiver.hpp"

class PipeFileReceiver : public FileReceiver
{
    int m_pipe_receiver_fd = 0;
    uint8_t m_buf[PIPE_BUF];
    std::string m_pipe_name;

    int RxTx(IpcCommand::ipc_command_tx_t tx, IpcCommand::ipc_command_rx_t rx_ok, IpcCommand::ipc_command_rx_t rx_nok);

public:
    PipeFileReceiver(IpcCommandReceiver *command_receiver, std::string pipe_name);
    virtual ~PipeFileReceiver();

    virtual file_rx_agreement_t connectionAgrrement(void);
    virtual file_rx_err_t receiveFile(const char *output_path);

private:
    FRIEND_TEST(IpcPipeTest, TestConstructor);
    FRIEND_TEST(IpcPipeTest, TestFileSendingReceivingSmallFile);
    FRIEND_TEST(IpcPipeTest, TestFileSendingReceivingLargeFile);
    FRIEND_TEST(IpcPipeTest, TestWrongFileAgreement);
};

