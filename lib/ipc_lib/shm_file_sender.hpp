#pragma once
#include <string>
#include <limits.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include "file_sender.hpp"

class ShmFileSender : public FileSender
{
private:
    sem_t *m_semaphore_handle_prod = nullptr;
    sem_t *m_semaphore_handle_cons = nullptr;
    caddr_t m_memptr = nullptr;
    int m_shm_file_descriptor = 0;
    uint64_t m_shm_buff_size;
    std::string m_shm_name;
    std::string m_semaphore_name_prod;
    std::string m_semaphore_name_cons;

public:
    ShmFileSender(IpcCommandSender *command_sender, std::string shm_name, std::string semaphore_name_producer, std::string semaphore_name_consumer, uint64_t buffer_size);
    ~ShmFileSender();

    virtual file_tx_agreement_t connectionAgrrement(std::string file_name, std::string file_extension, size_t file_size);
    virtual file_tx_err_t sendFile(const char * file_path);
};