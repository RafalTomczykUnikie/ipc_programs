#include <unistd.h>
#include <stdio.h>
#include <iostream>

#include "domain_socket_server.hpp"
#include "ipc_command_receiver.hpp"



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

    auto commander = IpcCommandReceiver(&socket);

    IpcCommand::IpcCommandTx tx;
    IpcCommand::IpcCommandRx rx;

    commander.receiveCommand(&tx);
    
    if(tx.command == IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK)
    {
        rx.response = IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK;
        cout << "command received properly!" << endl;
        commander.sendResponse(rx);
    }

    while(1)
    {
        IpcCommand::IpcCommandTx tx;
        IpcCommand::IpcCommandRx rx;

        commander.receiveCommand(&tx);

        if(tx.command == IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK)
        {
            rx.response = IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK;
            cout << "command received properly!" << endl;
            commander.sendResponse(rx);
        }
    }
}