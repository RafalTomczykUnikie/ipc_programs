
#include <ostream>
#include <fstream>
#include <memory>

#include <glog/logging.h>
#include "domain_socket_client.hpp"
#include "ipc_command_sender.hpp"
#include "pipe_file_sender.hpp"
#include "queue_file_sender.hpp"
#include "command_line_parser.hpp"
#include "shm_file_sender.hpp"
#include "message_file_sender.hpp"

using namespace std;

int main(int argc, char* argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = 1;
    FLAGS_minloglevel = 0;
    FLAGS_colorlogtostderr = 1;

    std::string file_d;
    std::shared_ptr<FileSender> file_sender = nullptr;

    CommandLineParser parser(argv[0]);

    parser.addOption({"-h", "--help", "prints help message", true});
    parser.addOption({"-m", "--messages", "program use messages as IPC METHOD", true});
    parser.addOption({"-q", "--queue", "program use queue as IPC METHOD", true});
    parser.addOption({"-p", "--pipe", "program use pipe as IPC METHOD", true});
    parser.addOption({"-s", "--shm", "program use shared buffer as IPC METHOD", false, "buffer size"});
    parser.addOption({"-f", "--file", "file that will be copied", false, "file name"});
    
    parser.parseOptions(argc, argv);
    if(parser.isErrorFound())
    {
        LOG(ERROR) << "Error found during arguments parsing! Aborting!";
        return EXIT_FAILURE;
    }

    if(parser.isOptionFound("-h"))
    {
        parser.printHelp();
        return 0;
    }

    if(parser.isOptionFound("-f"))
    {
        file_d = parser.getOptionValue("-f");
        LOG(INFO) << "File that will be copied is: " << file_d;
    }
    else
    {
        LOG(ERROR) << "No file specified. Exiting";
        return 0;
    }

    auto client = UnixDomainSocketClient(SV_CLIENT_SOCK_PATH, SOCK_DGRAM);
    LOG(INFO) << "Client is created" << endl;

    client.buildAddress();
    client.bind();
    client.setServerAddress(SV_SERVER_SOCK_PATH);

    LOG(INFO) << "Server address is set up!" << endl;

    auto commander = IpcCommandSender(&client);

    LOG(INFO) << "Command sender is working!" << endl;

    while(client.connect())
    {
        LOG(WARNING) << "Trying to connect to receiver server..." << endl;
        sleep(1); 
    }

    if(parser.isOptionFound("-m"))
    {
        file_sender = std::make_shared<MessageFileSender>(&commander, "/test_message");
    }
    else if(parser.isOptionFound("-q"))
    {
        file_sender = std::make_shared<QueueFileSender>(&commander, "/test_queue");
    }
    else if(parser.isOptionFound("-p"))
    {
        file_sender = std::make_shared<PipeFileSender>(&commander, "/tmp/testpipe");
    }
    else if(parser.isOptionFound("-s"))
    {
        try 
        {
            auto num = std::stoul(parser.getOptionValue("-s"));
            file_sender = std::make_shared<ShmFileSender>(&commander, "shm_buffer", "shm_sem_prod", "shm_sem_cons", num);
        } 
        catch(const std::exception &e)
        {
            LOG(ERROR) << "Exception detected in buffer size data. Aborting!";
            LOG(ERROR) << e.what();
            return EXIT_FAILURE;
        }       
    }
    else
    {
        file_sender = std::make_shared<PipeFileSender>(&commander, "/tmp/testpipe");
        LOG(INFO) << "No IPC METHOD specified, pipe is used as default!";
    }

    auto command = IpcCommandFactory::getCommand(IpcCommand::ipc_command_tx_t::IPC_CONNECTION_CHECK);
    auto response = IpcCommandFactory::getEmptyResponse();

    auto c_rslt = commander.sendCommand(command);
    auto r_rslt = commander.receiveResponse(&response);

    if(c_rslt != IpcCommandSender::command_send_error_t::COMMAND_SENT_OK ||
       r_rslt != IpcCommandSender::command_response_error_t::RESPONSE_OK ||
       response.response != IpcCommand::IPC_CONNECTION_OK)
    {
        return EXIT_FAILURE;
    }


    std::string file_path(file_d);

    auto name_pos = file_path.find_last_of('/');
    auto extension_pos = file_path.find_last_of('.');
    auto file_name = file_path.substr(name_pos + 1, extension_pos - name_pos - 1);
    auto file_extension = extension_pos != std::string::npos ? file_path.substr(extension_pos + 1) : std::string("");
    
    LOG(INFO) << "Loading file..." << std::endl;

    ifstream file (file_path.data(), ios::binary);
    uint64_t size = 0;

    if(file.is_open())
    {
        streampos begin,end;
        begin = file.tellg();
        file.seekg (0, ios::end);
        end = file.tellg();
        file.close();
        size = end - begin;
        file.close();
    }
    else
    {
        LOG(ERROR) << "Cannot open a file!";
        return 1;
    }

    LOG(INFO) << "File name is: -> " << file_name << " and file extension is -> " << file_extension << " with file size -> "<< size << std::endl;
    
    auto a_err = file_sender->connectionAgrrement(file_name, file_extension, size);
    if(a_err)
    {
        LOG(ERROR) << "Problem with file transfer agreement. Aborting.";
        return EXIT_FAILURE;
    }
    
    auto r_err = file_sender->sendFile(file_path.data());
    if(r_err)
    {
        LOG(ERROR) << "Error during file sending";
        return EXIT_FAILURE;
    }
   
    return 0;
}