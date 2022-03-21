#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <glog/logging.h>
#include "domain_socket_client.hpp"
#include "ipc_command_sender.hpp"
#include "pipe_file_sender.hpp"

using namespace std;

int main(int argc, char* argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = 1;
    FLAGS_minloglevel = 0;
    FLAGS_colorlogtostderr = 1;
    
    auto client = UnixDomainSocketClient(SV_CLIENT_SOCK_PATH, SOCK_DGRAM);
    LOG(INFO) << "Client is created" << endl;

    client.buildAddress();
    client.bind();
    client.setServerAddress(SV_SERVER_SOCK_PATH);

    LOG(INFO) << "Server address is set up!" << endl;

    auto commander = IpcCommandSender(&client);

    LOG(INFO) << "Command sender is working!" << endl;

    PipeFileSender pipe(&commander);

    
    while(client.connect())
    {
        LOG(WARNING) << "Trying to connect to receiver server..." << endl;
        sleep(1); 
    }

    client.close();

    pipe.connectionAgrrement("file", "txt", 10000);

    while(1)
    {   
        sleep(1);
    } 

   
    return 0;
}