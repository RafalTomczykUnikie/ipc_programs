#pragma once

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>

class IpcCommand
{

public:
    enum ipc_command_tx_t : int
    {
        IPC_CONNECTION_CHECK,
        IPC_SEND_FILE_METADATA,
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
        IPC_FILE_METADATA_OK,
        IPC_FILE_DESCRIPTOR_OK,
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
        char file_name[200] = {0}; 
        char file_extension[50] = {0};

        IpcCommandTx() :
            command(IPC_CONNECTION_CHECK),
            file_size(0)
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
};


struct IpcCommandFactory
{
    static IpcCommand::IpcCommandRx getEmptyResponse(void)
    {
        return IpcCommand::IpcCommandRx();
    }

    static IpcCommand::IpcCommandTx getEmptyCommand(void)
    {
        return IpcCommand::IpcCommandTx();
    }

    static IpcCommand::IpcCommandTx getCommand(IpcCommand::ipc_command_tx_t command, std::string file_name = "", std::string file_extension = "", size_t file_size = 0)
    {
        IpcCommand::IpcCommandTx com;
        com.command = command;
        memcpy(com.file_name, file_name.data(), file_name.length());
        memcpy(com.file_extension, file_extension.data(), file_extension.length());
        com.file_size = file_size;
        return com;
    }
};