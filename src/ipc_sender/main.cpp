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
#include "domain_socket_client.hpp"

using namespace std;

int main(int argc, char* argv[])
{
    // Fancy command line processing skipped for brevity,
    const auto ipc_receiver_name = "ipc_receiver";
    
    int x = 10;
    
    auto client = UnixDomainSocketClient(SV_CLIENT_SOCK_PATH, SOCK_DGRAM);

    cout << client.buildAddress() << endl;
    cout << client.bind() << endl;
    client.setServerAddress(SV_SERVER_SOCK_PATH);

    cout << "Connected!" << endl;

    char buffer[100] = "hello world from another process\r\n";
    uint32_t len = 100;
    
    // while(client.connect())
    //     cout << "Connecting..." << endl;

    int pfd[2] = {0,0};
    client.close();
    cout << client.connect() << endl;
    cout << client.sendFileDesc(pfd[0]) << endl;
    cout << client.bind() << endl;


    while(1)
    {      
    client.sendMessage((uint8_t*)buffer, len);
    client.recvMessage((uint8_t*)buffer, &len);
    sleep(1);
    } 

   
    return 0;
}