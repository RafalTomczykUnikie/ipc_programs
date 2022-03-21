#include <string>
#include <exception>
#include <stdexcept>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>

#include <glog/logging.h>

#include "domain_socket_server.hpp"

UnixDomainSocketServer::UnixDomainSocketServer(const char * socket_path, int socket_type) : 
    m_socket_path(socket_path),
    m_socket_type(socket_type),
    m_socket_error(NO_ERROR)
{
    m_socket_state = socket_server_state_t::CLOSED;
    m_sfd = -1;
    ::memset(&m_addr, 0, sizeof(sockaddr_un));
}

UnixDomainSocketServer::~UnixDomainSocketServer(void)
{
    ::close(m_sfd);
    ::remove(m_socket_path.data());
}

UnixDomainSocketServer::socket_error_t UnixDomainSocketServer::setClientAddress(std::string client_path)
{
    memset(&m_client_addr, 0, sizeof(sockaddr_un));
    m_client_addr.sun_family = AF_UNIX;
    strncpy(m_client_addr.sun_path, client_path.data(), client_path.length());
    m_client_addr.sun_path[0] = 0;
}

UnixDomainSocketServer::socket_error_t UnixDomainSocketServer::buildAddress(void)
{
    if(m_socket_state != socket_server_state_t::CLOSED)
    {
        m_socket_error = WRONG_STATE_ERROR;
        return m_socket_error;
    }

    auto result = remove(m_socket_path.data());

    if(result == -1 && errno != ENOENT)
    {
        LOG(ERROR) << ("Cannot remove socket path");
        m_socket_state = socket_server_state_t::FAULT;
        m_socket_error = PATH_REMOVE_ERROR;
        return m_socket_error;
    }

    memset(&m_addr, 0, sizeof(sockaddr_un));
    m_addr.sun_family = AF_UNIX;
    strncpy(m_addr.sun_path, m_socket_path.data(), m_socket_path.length());

    return m_socket_error;
}

UnixDomainSocketServer::socket_error_t UnixDomainSocketServer::bind(void)
{
    if(m_socket_state != socket_server_state_t::CLOSED)
    {
        m_socket_error = WRONG_STATE_ERROR;
        return m_socket_error;
    }
    
    m_sfd = socket(AF_UNIX, m_socket_type, 0);
    
    if(m_sfd == -1)
    {
        LOG(ERROR) << "Cannot create socket";
        m_socket_state = socket_server_state_t::FAULT;
        m_socket_error = CREATE_ERROR;
        return m_socket_error;
    }

    auto result = ::bind(m_sfd, reinterpret_cast<sockaddr*>(&m_addr), sizeof(sockaddr_un));

    if(result == -1)
    {
        LOG(ERROR) << "Cannot bind to socket";
        m_socket_state = socket_server_state_t::FAULT;
        m_socket_error = BIND_ERROR;
        return m_socket_error;
    }

    m_socket_state = socket_server_state_t::OPEN;

    return m_socket_error;
}

int UnixDomainSocketServer::getSocketType(void)
{
    return m_socket_type;
}

UnixDomainSocketServer::socket_server_state_t UnixDomainSocketServer::getSocketState(void)
{
    return m_socket_state;
}

UnixDomainSocketServer::socket_error_t UnixDomainSocketServer::receiveFileDesc(int *fileDescriptor)
{
    union {
        char   buf[CMSG_SPACE(sizeof(int))];
        struct cmsghdr align;
    } controlMsg;

    msghdr msgh;
    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;

    iovec iov;

    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;

    msgh.msg_control = controlMsg.buf;
    msgh.msg_controllen = sizeof(controlMsg.buf);

    LOG(INFO) << "receiving\r\n";

    auto nr = recvmsg(m_sfd, &msgh, 0);
    if (nr == -1)
    {
        LOG(ERROR) << "Cannot receive data properly!\n";
        m_socket_error = DATA_RECEIVE_ERROR;
        return m_socket_error;
    }

    cmsghdr *cmsgp = CMSG_FIRSTHDR(&msgh);

    if (cmsgp == NULL || cmsgp->cmsg_len != CMSG_LEN(sizeof(int)))
    {
        LOG(ERROR) << "bad cmsg header / message length\n";
        m_socket_error = WRONG_MSG_HEADER;
        return m_socket_error;
    }
    if (cmsgp->cmsg_level != SOL_SOCKET)
    {
        LOG(ERROR) << "cmsg_level != SOL_SOCKET\n";
        m_socket_error = WRONG_MSG_HEADER;
        return m_socket_error;
    }
    if (cmsgp->cmsg_type != SCM_RIGHTS)
    {
        LOG(ERROR) << "cmsg_type != SCM_RIGHTS\n";
        m_socket_error = WRONG_MSG_HEADER;
        return m_socket_error;
    }

    if(fileDescriptor != nullptr)
    {
        memcpy(fileDescriptor, CMSG_DATA(cmsgp), sizeof(int));
        LOG(ERROR) << "Received FD -> " << *fileDescriptor;
    }

    return m_socket_error;
}


UnixDomainSocketServer::socket_error_t UnixDomainSocketServer::sendMessage(uint8_t *data, uint32_t len)
{
    sendto(m_sfd, data, len, 0, reinterpret_cast<sockaddr*>(&m_client_addr), sizeof(sockaddr_un));
    return m_socket_error;
}

UnixDomainSocketServer::socket_error_t UnixDomainSocketServer::recvMessage(uint8_t *data, uint32_t *len)
{
    socklen_t addrlen = sizeof(sockaddr_un);
    *len = recvfrom(m_sfd, data, *len, 0, reinterpret_cast<sockaddr*>(&m_client_addr), &addrlen);
    return m_socket_error;
}