#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <glog/logging.h>

#include <string>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <mutex>
#include <algorithm>

#include "message_file_sender.hpp"
#include "message_file_receiver.hpp"
#include "ipc_command_sender.hpp"
#include "ipc_command_receiver.hpp"
#include "domain_socket_client.hpp"
#include "domain_socket_server.hpp"
#include "pipe_file_receiver.hpp"

using namespace std;

class IpcMessageTest : public ::testing::Test
{
protected:
    void SetUp(void) override;
    void TearDown(void) override;

    UnixDomainSocketClient * m_client;
    UnixDomainSocketServer * m_server;
    IpcCommandSender * m_comand_sender;
    IpcCommandReceiver * m_comand_receiver;
    FileReceiver * m_file_receiver;
    FileSender * m_file_sender;

};

void IpcMessageTest::SetUp(void)
{
    FLAGS_alsologtostderr = 1;
    FLAGS_minloglevel = 3;
    FLAGS_colorlogtostderr = 1;

    m_server = new UnixDomainSocketServer(SV_SERVER_SOCK_PATH, SOCK_DGRAM);
    m_server->buildAddress();
    m_server->bind();
    m_server->setClientAddress(SV_CLIENT_SOCK_PATH);

    m_client = new UnixDomainSocketClient(SV_CLIENT_SOCK_PATH, SOCK_DGRAM);
    m_client->buildAddress();
    m_client->bind();
    m_client->setServerAddress(SV_SERVER_SOCK_PATH);

    m_comand_receiver = new IpcCommandReceiver(m_server);
    m_comand_sender = new IpcCommandSender(m_client);

    m_file_sender = new MessageFileSender(m_comand_sender, "/test_queue");
    m_file_receiver = new MessageFileReceiver(m_comand_receiver, "/test_queue");    
}

void IpcMessageTest::TearDown(void)
{
    delete m_server;
    delete m_client;
    delete m_comand_receiver;
    delete m_comand_sender;
    delete dynamic_cast<MessageFileReceiver *>(m_file_receiver);
    delete dynamic_cast<MessageFileSender *>(m_file_sender);
}

TEST_F(IpcMessageTest, TestConstructor)
{
    auto *snd = dynamic_cast<MessageFileSender *>(m_file_sender);
    auto *rcv = dynamic_cast<MessageFileReceiver *>(m_file_receiver);

    EXPECT_NE(snd->m_queue_file_descrptors, 0);
    EXPECT_NE(rcv->m_queue_file_descrptors, 0);
    EXPECT_EQ(snd->m_queue_name, "/test_queue");
    EXPECT_EQ(rcv->m_queue_name, "/test_queue");
    EXPECT_NE(snd->m_command_sender, nullptr);
    EXPECT_NE(rcv->m_command_receiver, nullptr);
    
    EXPECT_NE(snd->m_attrs.mq_maxmsg, 0);
    EXPECT_NE(snd->m_attrs.mq_msgsize, 0);
    EXPECT_NE(rcv->m_attrs.mq_maxmsg, 0);
    EXPECT_NE(rcv->m_attrs.mq_msgsize, 0);
}

TEST_F(IpcMessageTest, TestFileSendingReceivingSmallFile)
{
    static constexpr auto data_size = 4096;
    srand (time(NULL));

    string in_file_name = "tmp_file_in.txt";
    string out_file_name = "tmp_file_out.txt";
    ofstream dummy_file;

    dummy_file.open(in_file_name.c_str(), ostream::binary);

    if(!dummy_file.is_open())
        throw runtime_error("cannot open input file! aborting.");

    std::vector<char> in_file_data(data_size, 0u);

    for(auto &element : in_file_data)
    {
        element = char((rand() % (127u - (' '))) + (' '));        
    }

    dummy_file.write(in_file_data.data(), in_file_data.size());
    dummy_file.close();

    ifstream in_file(in_file_name.c_str(), ios::binary);
    in_file.seekg(0, ios::end);
    int in_file_size = in_file.tellg();
    in_file.seekg(0);

    auto sender = [&](){
        m_file_sender->connectionAgrrement("tmp_file_in", "txt", data_size);
        m_file_sender->sendFile(in_file_name.c_str());
    };

    auto receiver = [&](){
        m_file_receiver->connectionAgrrement();
        m_file_receiver->receiveFile(out_file_name.c_str());
    };
    
    thread sn(sender);
    thread rv(receiver);

    sn.join();
    rv.join();

    auto file_exists = [&](const std::string& name) {
        struct stat buffer;   
        return (stat(name.c_str(), &buffer) == 0);
    };

    EXPECT_EQ(file_exists(out_file_name.c_str()), true);

    ifstream out_file(out_file_name.c_str(), ifstream::binary);
    out_file.seekg(0, ios::end);
    int out_file_size = out_file.tellg();
    out_file.seekg(0);

    EXPECT_EQ(out_file_size, in_file_size);

    out_file.open(out_file_name.c_str(), fstream::in | fstream::binary);
    if(!out_file.is_open())
    {
        throw runtime_error("cannot open output file! aborting.");
    }

    vector<char> out_file_data;
    istream_iterator<char> it_begin(out_file), it_end;

    copy(it_begin, it_end, back_inserter(out_file_data));

    for(auto idx = 0u; idx < out_file_data.size(); ++idx)
    {
        EXPECT_EQ(out_file_data[idx], in_file_data[idx]);
    }

    remove(in_file_name.c_str());
    remove(out_file_name.c_str());
}

TEST_F(IpcMessageTest, TestFileSendingReceivingLargeFile)
{
    constexpr auto data_size = (1u << 30);
    constexpr auto vector_data_size = (1<<20);
    auto data_left = data_size;

    srand (time(NULL));

    string in_file_name = "tmp_file_in.txt";
    string out_file_name = "tmp_file_out.txt";
    ofstream dummy_file;

    dummy_file.open(in_file_name.c_str(), ostream::binary);

    if(!dummy_file.is_open())
        throw runtime_error("cannot open input file! aborting.");   
    
    std::vector<char> in_file_data(vector_data_size);

    for(auto &element : in_file_data)
    {
        element = char((rand() % (127u - (' '))) + (' '));        
    }

    while(data_left)
    {
        dummy_file.write(in_file_data.data(), in_file_data.size());
        data_left -= vector_data_size;
    }
   
    dummy_file.close();

    ifstream in_file(in_file_name.c_str(), ios::binary);
    in_file.seekg(0, ios::end);
    int in_file_size = in_file.tellg();

    auto sender = [&](){
        m_file_sender->connectionAgrrement("tmp_file_in", "txt", data_size);
        m_file_sender->sendFile(in_file_name.c_str());
    };

    auto receiver = [&](){
        m_file_receiver->connectionAgrrement();
        m_file_receiver->receiveFile(out_file_name.c_str());
    };
    
    thread sn(sender);
    thread rv(receiver);

    sn.join();
    rv.join();

    auto file_exists = [&](const std::string& name) {
        struct stat buffer;   
        return (stat(name.c_str(), &buffer) == 0);
    };

    EXPECT_EQ(file_exists(out_file_name.c_str()), true);

    ifstream out_file(out_file_name.c_str(), fstream::in | ifstream::binary);
    out_file.seekg(0, ios::end);
    int out_file_size = out_file.tellg();
    

    EXPECT_EQ(out_file_size, in_file_size);

    out_file.open(out_file_name.c_str(), fstream::in | fstream::binary);
    if(!out_file.is_open())
    {
        throw runtime_error("cannot open output file! aborting.");
    }

    istream_iterator<char> in_it_begin(in_file), in_it_end;
    istream_iterator<char> out_it_begin(out_file), out_it_end;

    in_file.seekg(0, ifstream::beg);
    out_file.seekg(0, ifstream::beg);

    auto is_equal = equal(std::istreambuf_iterator<char>(in_file.rdbuf()),
                          std::istreambuf_iterator<char>(),
                          std::istreambuf_iterator<char>(out_file.rdbuf()));

    EXPECT_EQ(is_equal, true);

    remove(in_file_name.c_str());
    remove(out_file_name.c_str());
}

TEST_F(IpcMessageTest, TestWrongFileAgreement)
{
    auto temp_file_receiver = new PipeFileReceiver(m_comand_receiver, "test_pipe");

    static constexpr auto data_size = 4096;
    srand (time(NULL));

    string in_file_name = "tmp_file_in.txt";
    string out_file_name = "tmp_file_out.txt";
    ofstream dummy_file;

    dummy_file.open(in_file_name.c_str(), ostream::binary);

    if(!dummy_file.is_open())
        throw runtime_error("cannot open input file! aborting.");

    std::vector<char> in_file_data(data_size, 0u);

    for(auto &element : in_file_data)
    {
        element = char((rand() % (127u - (' '))) + (' '));        
    }

    dummy_file.write(in_file_data.data(), in_file_data.size());
    dummy_file.close();

    ifstream in_file(in_file_name.c_str(), ios::binary);
    in_file.seekg(0, ios::end);
    int in_file_size = in_file.tellg();
    in_file.seekg(0);

    FileSender::file_tx_agreement_t err_snd;
    FileReceiver::file_rx_agreement_t err_rcv;

    auto sender = [&](){
        auto err_snd = m_file_sender->connectionAgrrement("tmp_file_in", "txt", data_size);
        if(!err_snd)
            m_file_sender->sendFile(in_file_name.c_str());
    };

    auto receiver = [&](){
        err_rcv= temp_file_receiver->connectionAgrrement();
        if(!err_rcv)
            temp_file_receiver->receiveFile(out_file_name.c_str());
    };
    
    thread sn(sender);
    thread rv(receiver);

    sn.join();
    rv.join();

    EXPECT_NE(err_snd, 0);
    EXPECT_NE(err_rcv, 0);

    delete temp_file_receiver;
}