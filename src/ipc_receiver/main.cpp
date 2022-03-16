#include <unistd.h>
#include <stdio.h>
#include <iostream>

#include "domain_socket_server.hpp"

using namespace std;

int main(int argc, char* argv[])
{
    auto socket = UnixDomainSocketServer(SV_SERVER_SOCK_PATH, SOCK_DGRAM);

    cout << socket.buildAddress() << endl;
    cout << socket.bind() << endl;
    socket.setClientAddress(SV_CLIENT_SOCK_PATH);

    char buffer[100];
    uint32_t len = 100;
    int fd = 0;

    socket.receiveFileDesc(&fd);
    socket.bind();

    while(1)
    {
        socket.recvMessage((uint8_t *) buffer, &len);
        socket.sendMessage((uint8_t *) buffer, len);
        sleep(1);        
    }
}