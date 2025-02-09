#include <sys/socket.h>
#include <variant>
#include "Socket.h"

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

