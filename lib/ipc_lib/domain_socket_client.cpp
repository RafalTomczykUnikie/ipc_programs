#include <string>
#include <exception>
#include <stdexcept>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>

#include <glog/logging.h>
#include "domain_socket_client.hpp"

UnixDomainSocketClient::UnixDomainSocketClient(const char * socket_path, int socket_type) :
    m_socket_path(socket_path),
    m_socket_type(socket_type),
    m_socket_error(NO_ERROR),
    m_sfd_client(0)
{
    m_socket_state = socket_client_state_t::DISCONNECTED;
    m_sfd = -1;
    ::memset(&m_addr, 0, sizeof(sockaddr_un));
}

UnixDomainSocketClient::~UnixDomainSocketClient()
{
    ::close(m_sfd);
    close();
    ::remove(m_socket_path.data());
}

UnixDomainSocketClient::socket_client_error_t UnixDomainSocketClient::bind(socket_bind_t bind_sel)
{
    if(m_socket_state == FAULT)
    {
        m_socket_error = WRONG_STATE_ERROR;
        return m_socket_error;
    }

    auto result = remove(m_socket_path.data());

    if(result == -1 && errno != ENOENT)
    {
        LOG(ERROR) << ("Cannot remove socket path");
        return m_socket_error;
    }
    
    m_sfd = socket(AF_UNIX, m_socket_type, 0);
    
    if(m_sfd == -1)
    {
        LOG(ERROR) << ("Cannot create socket");
        m_socket_state = FAULT;
        m_socket_error = CREATE_ERROR;
        return m_socket_error;
    }

    result = int(0);
    if(bind_sel == BIND_TO_CLIENT)
        result = ::bind(m_sfd, reinterpret_cast<sockaddr*>(&m_addr), sizeof(sockaddr_un));
    else
        result = ::bind(m_sfd, reinterpret_cast<sockaddr*>(&m_server_address), sizeof(sockaddr_un)); 
    
    if(result == -1)
    {
        LOG(ERROR) << ("Cannot bind to socket");
        m_socket_state = FAULT;
        m_socket_error = BIND_ERROR;
        return m_socket_error;
    }

    return m_socket_error; 
}

UnixDomainSocketClient::socket_client_error_t UnixDomainSocketClient::sendFileDesc(int fileDescriptor)
{
    union {
        char   buf[CMSG_SPACE(sizeof(int))];
        struct cmsghdr align;
    } controlMsg;

    msghdr msgh;
    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;

    int data = 12345;

    iovec iov;
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    iov.iov_base = &data;
    iov.iov_len = sizeof(int);

    msgh.msg_control = controlMsg.buf;
    msgh.msg_controllen = sizeof(controlMsg.buf);

    memset(controlMsg.buf, 0, sizeof(controlMsg.buf));

    struct cmsghdr *cmsgp = CMSG_FIRSTHDR(&msgh);
    cmsgp->cmsg_len = CMSG_LEN(sizeof(int));
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_RIGHTS;
    memcpy(CMSG_DATA(cmsgp), &fileDescriptor, sizeof(int));

    ssize_t ns = sendmsg(m_sfd_client, &msgh, 0);

    if (ns == -1)
    {
        LOG(ERROR) << ("Cannot send data properly!");
        m_socket_error = DATA_SEND_ERROR;
    }

    return m_socket_error;
}

void UnixDomainSocketClient::close(void)
{
    ::close(m_sfd_client);
}

UnixDomainSocketClient::socket_client_error_t UnixDomainSocketClient::buildAddress(void)
{
    if(m_socket_state != socket_client_state_t::DISCONNECTED)
    {
        m_socket_error = WRONG_STATE_ERROR;
        return m_socket_error;
    }

    memset(&m_addr, 0, sizeof(sockaddr_un));
    m_addr.sun_family = AF_UNIX;
    strncpy(m_addr.sun_path, m_socket_path.data(), m_socket_path.length());
    m_addr.sun_path[0] = 0;

    return m_socket_error;
}

UnixDomainSocketClient::socket_client_error_t UnixDomainSocketClient::setServerAddress(std::string server_path)
{
    memset(&m_server_address, 0, sizeof(sockaddr_un));
    m_server_address.sun_family = AF_UNIX;
    strncpy(m_server_address.sun_path, server_path.data(), server_path.length());
    m_addr.sun_path[0] = 0;
    return socket_client_error_t::NO_ERROR;
}

UnixDomainSocketClient::socket_client_error_t UnixDomainSocketClient::connect(void)
{
    if(m_socket_state == CONNECTED)
    {
        LOG(INFO) << "Socket is already connected";
        m_socket_error = NO_ERROR;
        return m_socket_error;
    }

    if(m_socket_state == FAULT)
    {
        m_socket_error = WRONG_STATE_ERROR;
        return m_socket_error;
    }

    m_sfd_client = socket(AF_UNIX, m_socket_type, 0);

    if (m_sfd == -1)
    {
        LOG(ERROR) << ("Cannot connect to socket with given path -> socket()");
        m_socket_error = CREATE_ERROR;
        return m_socket_error;
    }
    
    auto result = ::connect(m_sfd_client, (struct sockaddr *) &m_server_address, sizeof(struct sockaddr_un));

    if (result == -1) 
    {
        LOG(ERROR) << ("Cannot connect to socket with given path -> connect()");
        m_socket_error = CONNECTION_ERROR;
        return m_socket_error;
    }

    m_socket_state = socket_client_state_t::CONNECTED;

    return m_socket_error;
}

UnixDomainSocketClient::socket_client_error_t UnixDomainSocketClient::sendMessage(uint8_t *data, uint32_t len)
{
    auto leng = sendto(m_sfd, data, len, 0, reinterpret_cast<sockaddr*>(&m_server_address), sizeof(sockaddr_un));
    if(leng == -1)
    {
        m_socket_error = DATA_SEND_ERROR;
    }
    else
    {
        m_socket_error = NO_ERROR;
    }
    return m_socket_error;
}

UnixDomainSocketClient::socket_client_error_t UnixDomainSocketClient::recvMessage(uint8_t *data, uint32_t *len)
{
    socklen_t addrlen = sizeof(sockaddr_un);
    *len = recvfrom(m_sfd, data, *len, 0, reinterpret_cast<sockaddr*>(&m_server_address), &addrlen);
    return m_socket_error;
}