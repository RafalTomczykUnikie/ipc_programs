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
#include "ipc_command_sender.hpp"

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

    

    auto commander = IpcCommandSender(&client);

    IpcCommand::IpcCommandTx tx;
    IpcCommand::IpcCommandRx rx;
 
    tx.command = IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK;

    auto err = commander.sendCommand(tx);
    
    while (err)
    {
        cout << "Cannot send data to server! Retrying..." << endl;
        err = commander.sendCommand(tx);
        sleep(1);
    }
    

    commander.receiveResponse(&rx);
    if(rx.response == IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK)
    {
        cout << "Data properly exchanged over IPC socket" << endl;
    }
    else 
    {
        cout << "response ->" << rx.response << endl;
    }

    while(1)
    {   
        err = commander.sendCommand(tx);
        while (err)
        {
            cout << "Cannot send data to server! Retrying..." << endl;
            err = commander.sendCommand(tx);
            sleep(1);
        }
        commander.receiveResponse(&rx);
        if(rx.response == IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK)
        {
            cout << "Data properly exchanged over IPC socket" << endl;
        }
        else 
        {
            cout << "response ->" << rx.response << endl;
        }
        sleep(1);
    } 

   
    return 0;
}