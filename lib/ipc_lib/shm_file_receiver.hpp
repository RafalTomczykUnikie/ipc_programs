#pragma once
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include "file_receiver.hpp"

class ShmFileReceiver : public FileReceiver
{
    sem_t *m_semaphore_handle_prod = nullptr;
    sem_t *m_semaphore_handle_cons = nullptr;
    caddr_t m_memptr = nullptr;
    int m_shm_file_descriptor = 0;
    uint64_t m_shm_buff_size;
    std::string m_shm_name;
    std::string m_semaphore_name_prod;
    std::string m_semaphore_name_cons;

    

    int RxTx(IpcCommand::ipc_command_tx_t tx, IpcCommand::ipc_command_rx_t rx_ok, IpcCommand::ipc_command_rx_t rx_nok);

public:
    ShmFileReceiver(IpcCommandReceiver *command_receiver, std::string shm_name, std::string semaphore_name_producer, std::string semaphore_name_consumer, uint64_t buffer_size);
    ~ShmFileReceiver();
    virtual file_rx_agreement_t connectionAgrrement(void);
    virtual file_rx_err_t receiveFile(const char *output_path);
};