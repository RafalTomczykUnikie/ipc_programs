#include <glog/logging.h>

#include "shm_file_receiver.hpp"
#include "ipc_command.hpp"


ShmFileReceiver::ShmFileReceiver(IpcCommandReceiver *command_receiver, std::string shm_name, std::string semaphore_name_producer, std::string semaphore_name_consumer, uint64_t buffer_size) : 
    FileReceiver(command_receiver), 
    m_shm_buff_size(buffer_size),
    m_shm_name(shm_name),
    m_semaphore_name_prod(semaphore_name_producer),
    m_semaphore_name_cons(semaphore_name_consumer)
{
    constexpr auto flags = O_RDWR;
    constexpr auto perms = 0777;
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

    m_semaphore_handle_prod = sem_open(m_semaphore_name_prod.data(), flags, perms);
    if(m_semaphore_handle_prod == SEM_FAILED)
    {
        throw std::runtime_error("Cannot create producer sempahore with given name!");
    }

    m_semaphore_handle_cons = sem_open(m_semaphore_name_cons.data(), flags, perms);
    if(m_semaphore_handle_cons == SEM_FAILED)
    {
        throw std::runtime_error("Cannot create consumer sempahore with given name!");
    }
}

ShmFileReceiver::~ShmFileReceiver()
{
    munmap(m_memptr, m_shm_buff_size);
    close(m_shm_file_descriptor);
    sem_close(m_semaphore_handle_cons);
    sem_close(m_semaphore_handle_prod);
    shm_unlink(m_shm_name.data());
}

ShmFileReceiver::file_rx_agreement_t ShmFileReceiver::connectionAgrrement(void)
{
    auto res = RxTx(IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK, 
                    IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK,
                    IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR);

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    LOG(INFO) << "Connection agrrement send back";

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_SEND_OVER_SHARED_BUFFER, 
         IpcCommand::ipc_command_rx_t::IPC_SEND_OVER_SHARED_BUFFER_CONFIRMED,
         IpcCommand::ipc_command_rx_t::IPC_DIFFERENT_IPC_METHOD);

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    LOG(INFO) << "File will be send over shared memory - confirmation send back";

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_SEND_FILE_METADATA, 
         IpcCommand::ipc_command_rx_t::IPC_FILE_METADATA_OK,
         IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR);

    std::string full_file_name = m_file_name; 

    if(m_file_extension != "")
    {
        m_file_extension += "." + m_file_extension;
    }

    LOG(INFO) << "File metadata exchanged properly - confirmation sent back";
    LOG(INFO) << "File that will be send is -> " << full_file_name << " with size = " << m_file_size << " bytes";

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_SHM_BUFFER_SIZE, 
         IpcCommand::ipc_command_rx_t::IPC_SHM_BUFFER_SIZE_OK,
         IpcCommand::ipc_command_rx_t::IPC_SHM_BUFFER_SIZE_ERROR);

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK, 
         IpcCommand::ipc_command_rx_t::IPC_CONNECTION_OK,
         IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR);

    if(res)
        return file_rx_agreement_t::FILE_AGR_ERROR;
    
    return file_rx_agreement_t::FILE_AGR_OK;
}

int ShmFileReceiver::RxTx(IpcCommand::ipc_command_tx_t tx, IpcCommand::ipc_command_rx_t rx_ok, IpcCommand::ipc_command_rx_t rx_nok)
{
    auto command = IpcCommandFactory::getEmptyCommand();
    auto response = IpcCommandFactory::getEmptyResponse();
    
    auto c_rslt = m_command_receiver->receiveCommand(&command);

    if(c_rslt != IpcCommandReceiver::command_recv_error_t::COMMAND_RECV_OK)
    {
        return -1;
    }

    if(command.command != tx)
    {
        response.response = rx_nok;
    }
    else
    {
        response.response = rx_ok;

        if(command.command == IpcCommand::ipc_command_tx_t::IPC_SEND_FILE_METADATA)
        {
            m_file_name = std::string(command.file_name);
            m_file_extension = std::string(command.file_extension);
            m_file_size = command.file_descr.file_size;
        }
        else if(command.command == IpcCommand::ipc_command_tx_t::IPC_SHM_BUFFER_SIZE)
        {
            if(m_shm_buff_size != command.file_descr.file_shared_buffer_size)
            {
                response.response = rx_nok;
            }
        }        
    }

    auto r_rslt = m_command_receiver->sendResponse(response);

    if(r_rslt != IpcCommandReceiver::command_send_resp_error_t::RESPONSE_SENT_OK || 
       response.response == rx_nok)
    {
        return -1;
    }

    return 0;
}

    
ShmFileReceiver::file_rx_err_t ShmFileReceiver::receiveFile(const char *output_path)
{
    std::string out_path(output_path);

    auto res = RxTx(IpcCommand::ipc_command_tx_t::IPC_START_FILE_TRANSFER, 
                    IpcCommand::ipc_command_rx_t::IPC_START_FILE_TRANSFER_CONFIRMED,
                    IpcCommand::ipc_command_rx_t::IPC_UNEXPECTED_COMMAND_ERROR);

    if(res == -1)
    {
        return FILE_RCV_ERROR;
    }

    LOG(INFO) << "Starting file receiving...";

    auto file_descriptor = open(out_path.data(), O_CREAT | O_WRONLY, 0666);

    if(file_descriptor == -1)
    {
        LOG(ERROR) << "Cannot write to specified path!";
    }

    while(1)
    {
        sem_wait(m_semaphore_handle_prod);

        uint64_t *data_size_location = (uint64_t *)m_memptr;
        uint8_t *data_location = (uint8_t*)(m_memptr + sizeof(uint64_t));

        auto r = int(0);
        auto s = *data_size_location;
  
        if(s > 0)
        {         
            r = write(file_descriptor, data_location, s);
            if(r == -1)
            {
                LOG(ERROR) << "Error in writing to a file. Aborting!";
                close(file_descriptor);
                return FILE_RCV_ERROR;
            }
        }

        sem_post(m_semaphore_handle_cons);

        if(s < m_shm_buff_size)
        {
            LOG(INFO) << "End of file is reached, finishing...";
            break;
        }
    }
    
    close(file_descriptor);

    res = RxTx(IpcCommand::ipc_command_tx_t::IPC_ALL_DATA_SENT, 
               IpcCommand::ipc_command_rx_t::IPC_ALL_DATA_RECEIVED_OK,
               IpcCommand::ipc_command_rx_t::IPC_ALL_DATA_RECEIVED_ERROR);
    
    if(res == -1)
    {
        return FILE_RCV_ERROR;
    }

    LOG(INFO) << "File transfer is successful";

    return file_rx_err_t::FILE_RCV_OK;
}