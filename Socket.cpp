#include <sstream>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "Socket.h"

#include <iostream> // FIXME

using namespace posixcpp;

// work on this
// struct sockaddr_un serv_addr
struct sockaddr_storage ss;


Socket::Socket(int domain, int type, int protocol)
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

// FIXME - work in progress
stringvec_t Socket::getaddrinfo(const std::string& host, int addrFam)
{
    struct addrinfo hints{0};
    struct addrinfo *res;
    hints.ai_family = addrFam;
    hints.ai_socktype = SOCK_STREAM; // Or SOCK_DGRAM for UDP
    hints.ai_flags = AI_PASSIVE;    // For binding (server), if needed

    int status;
    if ((status = ::getaddrinfo(host.c_str(), NULL /*port*/, &hints, &res)) != 0)
    {
        std::ostringstream oss;
        oss << "getaddrinfo: " << gai_strerror(status);
        throw std::runtime_error(oss.str());
    }

    std::vector<std::string> ret;
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
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        ret.push_back(ipstr);
    }
    freeaddrinfo(res); // Free the linked list
    return ret;
}

