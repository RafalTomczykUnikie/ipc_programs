#include <unistd.h>
#include <stdio.h>
#include <iostream>

#include "domain_socket_server.hpp"
#include "ipc_command_receiver.hpp"
#include "pipe_file_receiver.hpp"
#include "command_line_parser.hpp"
#include "queue_file_receiver.hpp"
#include "shm_file_receiver.hpp"

#include <glog/logging.h>

using namespace std;

int main(int argc, char* argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = 1;
    FLAGS_minloglevel = 0;
    FLAGS_colorlogtostderr = 1;

    FileReceiver * file_receiver = nullptr;

    std::string file_d;
    CommandLineParser parser(argv[0]);

    parser.addOption({"-h", "--help", "prints help message", true});
    parser.addOption({"-m", "--messages", "program use messages as IPC METHOD", true});
    parser.addOption({"-q", "--queue", "program use queue as IPC METHOD", true});
    parser.addOption({"-p", "--pipe", "program use pipe as IPC METHOD", true});
    parser.addOption({"-s", "--shm", "program use shared buffer as IPC METHOD", false});
    parser.addOption({"-f", "--file", "file that will be copied", false});
    
    parser.parseOptions(argc, argv);

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

    PipeFileReceiver pipe(&commander);
    QueueFileReceiver queue(&commander, "/test_queue");
    ShmFileReceiver shm(&commander, "shm_buffer", "shm_sem_prod", "shm_sem_cons", 8096);


    if(parser.isOptionFound("-m"))
    {
        file_receiver = &queue;
    }
    else if(parser.isOptionFound("-q"))
    {
        file_receiver = &queue;
    }
    else if(parser.isOptionFound("-p"))
    {
        file_receiver = &pipe;
    }
    else if(parser.isOptionFound("-s"))
    {
        file_receiver = &shm;
    }
    else
    {
        file_receiver = &pipe;
        LOG(INFO) << "No IPC METHOD specified, pipe is used as default!";
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