#pragma once

#include <cstdlib>
#include <cstdint>

class IpcCommand
{

public:

    enum ipc_command_tx_t : int
    {
        IPC_CONNECTION_CHECK,
        IPC_SEND_FILE_DESCRIPTOR,
        IPC_SEND_OVER_PIPE,
        IPC_SEND_OVER_MESSAGE,
        IPC_SEND_OVER_MESSAGE_QUEUE,
        IPC_SEND_OVER_SHARED_BUFFER,
        IPC_ALL_DATA_SENT,
        IPC_DATA_SEND_ERROR,
        IPC_SEND_DATA_AGAIN
    };

    enum ipc_command_rx_t : int
    {
        IPC_CONNECTION_OK,
        IPC_SEND_OVER_PIPE_CONFIRMED,
        IPC_SEND_OVER_MESSAGE_CONFIRMED,
        IPC_SEND_OVER_MESSAGE_QUEUE_CONFIRMED,
        IPC_SEND_OVER_SHARED_BUFFER_CONFIRMED,
        IPC_DIFFERENT_IPC_METHOD,
        IPC_ALL_DATA_RECEIVED_OK,
        IPC_ALL_DATA_RECEIVED_ERROR,
        IPC_SEND_DATA_AGAIN_READY
    };

    struct IpcCommandTx
    {
        ipc_command_tx_t command;
        size_t file_size;
        char *file_name;
        size_t file_name_size;

        IpcCommandTx() :
            command(IPC_CONNECTION_CHECK),
            file_size(0),
            file_name(nullptr),
            file_name_size(0)
        {

        };



    };


    struct IpcCommandRx
    {
        ipc_command_rx_t response;
        IpcCommandRx() :
        response(IPC_CONNECTION_OK)
        {
            
        };
    };
private:

    IpcCommandTx m_last_tx_command;
    IpcCommandRx m_last_rx_command;
};
