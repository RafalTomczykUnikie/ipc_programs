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
        IPC_START_FILE_TRANSFER,
        IPC_ALL_DATA_SENT,
        IPC_DATA_SEND_ERROR,
        IPC_SEND_DATA_AGAIN,
        IPC_SHM_BUFFER_SIZE
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
        IPC_START_FILE_TRANSFER_CONFIRMED,
        IPC_ALL_DATA_RECEIVED_OK,
        IPC_ALL_DATA_RECEIVED_ERROR,
        IPC_SEND_DATA_AGAIN_READY,
        IPC_UNEXPECTED_COMMAND_ERROR,
        IPC_SHM_BUFFER_SIZE_OK,
        IPC_SHM_BUFFER_SIZE_ERROR
    };

    union file_desc
    {
        size_t file_size;
        int file_descriptor;
        uint64_t file_shared_buffer_size;
    };

    struct IpcCommandTx
    {
        ipc_command_tx_t command;
        file_desc file_descr;
        char file_name[200] = {0}; 
        char file_extension[50] = {0};
        IpcCommandTx() :
            command(IPC_CONNECTION_CHECK),
            file_descr{0}
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
        com.file_descr.file_size = file_size;
        return com;
    }

    static IpcCommand::IpcCommandTx getCommand(IpcCommand::ipc_command_tx_t command, int file_descriptor)
    {
        IpcCommand::IpcCommandTx com;
        com.command = command;
        com.file_descr.file_descriptor = file_descriptor;
        return com;
    }

    static IpcCommand::IpcCommandTx getCommand(IpcCommand::ipc_command_tx_t command, uint64_t shm_buffer_size)
    {
        IpcCommand::IpcCommandTx com;
        com.command = command;
        com.file_descr.file_shared_buffer_size = shm_buffer_size;
        return com;
    }
};