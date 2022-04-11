#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <memory>

#include "domain_socket_server.hpp"
#include "ipc_command_receiver.hpp"
#include "pipe_file_receiver.hpp"
#include "command_line_parser.hpp"
#include "queue_file_receiver.hpp"
#include "shm_file_receiver.hpp"
#include "message_file_receiver.hpp"

#include <glog/logging.h>

using namespace std;

int main(int argc, char* argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = 1;
    FLAGS_minloglevel = 0;
    FLAGS_colorlogtostderr = 1;

    std::shared_ptr<FileReceiver> file_receiver = nullptr;

    std::string file_d;
    CommandLineParser parser(argv[0]);

    parser.addOption({"-h", "--help", "prints help message", true});
    parser.addOption({"-m", "--messages", "program use messages as IPC METHOD", true});
    parser.addOption({"-q", "--queue", "program use queue as IPC METHOD", true});
    parser.addOption({"-p", "--pipe", "program use pipe as IPC METHOD", true});
    parser.addOption({"-s", "--shm", "program use shared buffer as IPC METHOD", false, "buffer size"});
    parser.addOption({"-f", "--file", "file that will be copied", false, "file name"});
    
    parser.parseOptions(argc, argv);

    if(parser.isErrorFound())
    {
        LOG(ERROR) << "Error found during arguments parsing! Aborting!";
        return EXIT_FAILURE;
    }

    if(parser.isOptionFound("-h"))
    {
        parser.printHelp();
        return 0;
    }

    if(parser.isOptionFound("-f"))
    {
        file_d = parser.getOptionValue("-f");
        LOG(INFO) << "File that will be stored is: " << file_d;
    }
    else
    {
        LOG(ERROR) << "No file specified. Exiting";
        return 0;
    }


    auto socket = UnixDomainSocketServer(SV_SERVER_SOCK_PATH, SOCK_DGRAM);
    LOG(INFO) << "Socket is opened" << endl;

    socket.buildAddress();
    socket.bind();
    socket.setClientAddress(SV_CLIENT_SOCK_PATH);

    auto commander = IpcCommandReceiver(&socket);


    LOG(INFO) << "Ipc commander created" << endl;

    if(parser.isOptionFound("-m"))
    {
        file_receiver = std::make_shared<MessageFileReceiver>(&commander, "/test_message");
    }
    else if(parser.isOptionFound("-q"))
    {
        file_receiver = std::make_shared<QueueFileReceiver>(&commander, "/test_queue");
    }
    else if(parser.isOptionFound("-p"))
    {
        file_receiver = std::make_shared<PipeFileReceiver>(&commander, "/tmp/testpipe");
    }
    else if(parser.isOptionFound("-s"))
    {
        auto num = 0ul;
        try 
        {
            num = std::stoul(parser.getOptionValue("-s"));
        } 
        catch(const std::exception &e)
        {
            LOG(ERROR) << "Exception detected in buffer size data. Aborting!";
            LOG(ERROR) << e.what();
            return EXIT_FAILURE;
        }       

        while(true)
        {
            try
            {
                file_receiver = std::make_shared<ShmFileReceiver>(&commander, "shm_buffer", "shm_sem_prod", "shm_sem_cons", num);
                break;
            }
            catch(const std::exception& e)
            {
                sleep(1);
                LOG(INFO) << e.what();
            }
        }    
    }
    else
    {
        file_receiver = std::make_shared<PipeFileReceiver>(&commander, "/tmp/testpipe");
        LOG(INFO) << "No IPC METHOD specified, pipe is used as default!";
    }

    auto command = IpcCommandFactory::getEmptyCommand();
    auto response = IpcCommandFactory::getEmptyResponse();
    
    auto c_rslt = commander.receiveCommand(&command);

    if(c_rslt != IpcCommandReceiver::command_recv_error_t::COMMAND_RECV_OK)
    {
        return EXIT_FAILURE;
    }

    if(command.command != IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK)
    {
        response.response = IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR;
    }

    auto r_rslt = commander.sendResponse(response);

    if(r_rslt != IpcCommandReceiver::command_send_resp_error_t::RESPONSE_SENT_OK || 
       response.response == IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR)
    {
        return EXIT_FAILURE;
    }

    auto a_err = file_receiver->connectionAgrrement();
    if(a_err)
    {
        LOG(ERROR) << "Problem with file transfer agreement. Aborting.";
        return EXIT_FAILURE;
    }

    auto r_err = file_receiver->receiveFile(file_d.data());
    if(r_err)
    {
        LOG(ERROR) << "Error during file reception";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}