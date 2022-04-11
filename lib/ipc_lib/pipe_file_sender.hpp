#pragma once
#include <limits.h>
#include <gtest/gtest_prod.h>

#include "file_sender.hpp"


class PipeFileSender : public FileSender
{
private:
    int m_pipe_receiver_fd = {0};
    uint8_t m_buf[PIPE_BUF];
    std::string m_pipe_name;

public:
    PipeFileSender(IpcCommandSender *command_sender, std::string pipe_name);
    virtual ~PipeFileSender();

    virtual file_tx_agreement_t connectionAgrrement(std::string file_name, std::string file_extension, size_t file_size);
    virtual file_tx_err_t sendFile(const char * file_path);

private:
    FRIEND_TEST(IpcPipeTest, TestConstructor);
    FRIEND_TEST(IpcPipeTest, TestFileSendingReceivingSmallFile);
    FRIEND_TEST(IpcPipeTest, TestFileSendingReceivingLargeFile);
    FRIEND_TEST(IpcPipeTest, TestWrongFileAgreement);
};