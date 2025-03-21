#include <cstring>
#include <sstream>
#include <vector>
#include <set>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "Socket.h"

using namespace posixcpp;

sockaddr_storage ss;

// work on this
// struct sockaddr_un serv_addr

Socket::Socket(int domain, int type, int protocol)
: m_domain(domain),
  m_type(type),
  m_protocol(protocol)
{
    int fd = socket(domain,type,protocol);
    PosixError::ASSERT(fd != -1);
    m_file = File(fd,"socket");
}

bool Socket::shutdown(int how)
{
    int r = ::shutdown(m_file.fd(),how);
    if (r==0)
    {
        return true;
    }
    
    if (errno == ENOTCONN)
    {
        return false;
    }
    throw PosixError("error in shutdown");
}

ssize_t Socket::send(const void *buf, size_t len, int flags) const
{
    ssize_t r = ::send(fd(),buf,len,flags);
    PosixError::ASSERT(r != -1);
    return r;
}

ssize_t Socket::recv(void *buf, size_t len, int flags)
{
    ssize_t r = ::recv(fd(),buf,len,flags);
    PosixError::ASSERT(r != -1);
    return r;
}

gai_vec_t Socket::getaddrinfo(const std::string& host, int *eaiVal)
{
    struct addrinfo hints{0};
    struct addrinfo *res;
    hints.ai_family = domain();
    hints.ai_socktype = SOCK_STREAM; // Or SOCK_DGRAM for UDP
    hints.ai_flags = AI_PASSIVE;    // For binding (server), if needed
    gai_vec_t ret;

    int status = ::getaddrinfo(host.c_str(), NULL /*port*/, &hints, &res);
    if (eaiVal)
    {
        *eaiVal = status;
    }
    if (status != 0)
    {
        return ret;
    }

    for (addrinfo* p = res; p != NULL; p = p->ai_next)
    {
        void *addr;
        char ipstr[INET6_ADDRSTRLEN];

        if (p->ai_family == AF_INET)
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
        }
        else // AF_INET6
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
        const char* rstr = inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        if (rstr == NULL)
        {
            continue;
        }
        gai_vec_t::value_type rr;
        rr.first = rstr;

        memcpy((void*)&rr.second,p->ai_addr,sizeof(sockaddr_storage));
        ret.push_back(rr);
    }
    freeaddrinfo(res); // Free the linked list
    return ret;
}

gai_vec_t Socket::getaddrinfo(const std::string& host, int domain, int *eaiVal)
{
    return Socket(domain, SOCK_STREAM).getaddrinfo(host,eaiVal);
}

ssize_t Socket::sendto(const void *buf, size_t len, int flags,
                   const struct sockaddr *dest_addr, socklen_t addrlen)
{
    return ::sendto(fd(),buf,len,flags,dest_addr,addrlen);
}
