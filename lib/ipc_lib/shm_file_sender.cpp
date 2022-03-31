#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#include <limits.h>
#include <glog/logging.h>
#include "shm_file_sender.hpp"

ShmFileSender::ShmFileSender(IpcCommandSender *command_sender, std::string shm_name, std::string semaphore_name_producer, std::string semaphore_name_consumer, uint64_t buffer_size) :
    FileSender(command_sender),
    m_shm_buff_size(buffer_size),
    m_shm_name(shm_name),
    m_semaphore_name_prod(semaphore_name_producer),
    m_semaphore_name_cons(semaphore_name_consumer)
{
    constexpr auto flags = O_RDWR | O_CREAT;
    constexpr auto perms = 0666;
    m_shm_file_descriptor = shm_open(m_shm_name.data(), flags, perms);
    if(m_shm_file_descriptor < 0)
    {
        throw std::runtime_error("Cannot open shared memory blockfile!");
    }

    m_memptr = reinterpret_cast<caddr_t>(mmap(NULL, m_shm_buff_size + sizeof(uint64_t), PROT_READ | PROT_WRITE, MAP_SHARED, m_shm_file_descriptor, 0));
    if (m_memptr == MAP_FAILED)
    {
        throw std::runtime_error("Cannot map memory buffer with given size!");
    }

    ftruncate(m_shm_file_descriptor, m_shm_buff_size + sizeof(uint64_t));

    sem_unlink(m_semaphore_name_prod.data());
    sem_unlink(m_semaphore_name_cons.data());

    m_semaphore_handle_prod = sem_open(m_semaphore_name_prod.data(), flags, perms, 0);
    if(m_semaphore_handle_prod == SEM_FAILED)
    {
        throw std::runtime_error("Cannot create producer sempahore with given name!");
    }

    m_semaphore_handle_cons = sem_open(m_semaphore_name_cons.data(), flags, perms, 1);
    if(m_semaphore_handle_cons == SEM_FAILED)
    {
        throw std::runtime_error("Cannot create consumer sempahore with given name!");
    }
}

ShmFileSender::~ShmFileSender()
{
    munmap(m_memptr, m_shm_buff_size);
    close(m_shm_file_descriptor);
    sem_close(m_semaphore_handle_cons);
    sem_close(m_semaphore_handle_prod);
    sem_unlink(m_semaphore_name_prod.data());
    sem_unlink(m_semaphore_name_cons.data());
    shm_unlink(m_shm_name.data());
}

ShmFileSender::file_tx_agreement_t ShmFileSender::connectionAgrrement(std::string file_name, std::string file_extension, size_t file_size)
{
    file_tx_agreement_t retVal = FILE_AGREMENT_OK;

    auto command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK);
    auto response = IpcCommandFactory::getEmptyResponse();

    auto c_rslt = m_command_sender->sendCommand(command);
    auto r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
       response.response != IpcCommand::IPC_CONNECTION_OK)
    {
        retVal = FILE_AGREMENT_ERROR;
        return retVal;
    }

    LOG(INFO) << "Connection agrrement ok";

    command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_SEND_OVER_SHARED_BUFFER);
    response = IpcCommandFactory::getEmptyResponse();

    c_rslt = m_command_sender->sendCommand(command);
    r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
        response.response != IpcCommand::ipc_command_rx_t::IPC_SEND_OVER_SHARED_BUFFER_CONFIRMED)
    {
        retVal = FILE_AGREMENT_ERROR;
        return retVal;
    }

    LOG(INFO) << "File will be send over shared memory";

    command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_SEND_FILE_METADATA, file_name, file_extension, file_size);
    response = IpcCommandFactory::getEmptyResponse();

    c_rslt = m_command_sender->sendCommand(command);
    r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
       response.response != IpcCommand::ipc_command_rx_t::IPC_FILE_METADATA_OK)
    {
        retVal = FILE_AGREMENT_ERROR;
        return retVal;
    }

    LOG(INFO) << "Checking connection again...";

    command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK);
    response = IpcCommandFactory::getEmptyResponse();

    c_rslt = m_command_sender->sendCommand(command);
    r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
        response.response != IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK)
    {
        retVal = FILE_AGREMENT_ERROR;
        return retVal;
    }

    LOG(INFO) << "Connection checked properly!";

    return retVal;
}

ShmFileSender::file_tx_err_t ShmFileSender::sendFile(const char * file_path)
{
    auto command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_START_FILE_TRANSFER);
    auto response = IpcCommandFactory::getEmptyResponse();

    auto c_rslt = m_command_sender->sendCommand(command);
    auto r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
       response.response != IpcCommand::ipc_command_rx_t::IPC_START_FILE_TRANSFER_CONFIRMED)
    {
        return FILE_SEND_ERROR;
    }  

    auto sendError = [&](void){
        command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_DATA_SEND_ERROR);
        response = IpcCommandFactory::getEmptyResponse();
        c_rslt = m_command_sender->sendCommand(command);
        r_rslt = m_command_sender->receiveResponse(&response);

        return FILE_SEND_ERROR;
    };

    LOG(INFO) << "Starting file transfer...";

    auto file_descriptor = open(file_path, O_RDONLY);

    if(file_descriptor == -1)
    {
        LOG(ERROR) << "Cannot open file specified";
        return sendError();
    }

    auto s = 0u;

    while(1)
    {
        sem_wait(m_semaphore_handle_cons);

        uint64_t *data_size_location = (uint64_t *)m_memptr;
        uint8_t *data_location = (uint8_t*)(m_memptr + sizeof(uint64_t));

        s = read(file_descriptor, data_location, m_shm_buff_size);
  
        *data_size_location = s;

        sem_post(m_semaphore_handle_prod);

        if(s < m_shm_buff_size)
        {
            LOG(INFO) << "End of file is reached, finishing...";
            break;
        }
    }

    close(file_descriptor);

    command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_ALL_DATA_SENT);
    response = IpcCommandFactory::getEmptyResponse();
    c_rslt = m_command_sender->sendCommand(command);
    r_rslt = m_command_sender->receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
       response.response != IpcCommand::ipc_command_rx_t::IPC_ALL_DATA_RECEIVED_OK)
    {
        return FILE_SEND_ERROR;
    } 

    LOG(INFO) << "File transfer is successful";

    return FILE_SEND_OK;
}