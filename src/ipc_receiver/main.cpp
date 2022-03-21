#include <unistd.h>
#include <stdio.h>
#include <iostream>

#include "domain_socket_server.hpp"
#include "ipc_command_receiver.hpp"
#include "pipe_file_receiver.hpp"

#include <glog/logging.h>

using namespace std;

int main(int argc, char* argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = 1;
    FLAGS_minloglevel = 0;
    FLAGS_colorlogtostderr = 1;

    auto socket = UnixDomainSocketServer(SV_SERVER_SOCK_PATH, SOCK_DGRAM);
    LOG(INFO) << "Socket is opened" << endl;

    socket.buildAddress();
    socket.bind();
    socket.setClientAddress(SV_CLIENT_SOCK_PATH);

    auto commander = IpcCommandReceiver(&socket);

    LOG(INFO) << "Ipc commander created" << endl;

    PipeFileReceiver pipe(&commander);

    pipe.connectionAgrrement();

    while(1)
    {
        sleep(1);
    }
}