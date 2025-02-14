#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include "File.h"
#include "PosixError.h"

namespace posixcpp
{
typedef std::vector<std::string> stringvec_t;

class Socket
{
    File m_file;
public:

    Socket(int domain, int type, int protocol=0);

    // Returns true if no errors, false if ENOTCONN error, throws PosixError otherwise
    bool shutdown(int how);

    int fd() const {return m_file.fd();};

    static stringvec_t getaddrinfo(const std::string& host, int addrFam=AF_INET);
};

// FIXME - work on this
class ClientSocket : public Socket
{
protected:
    struct sockaddr_storage m_sockaddr_storage;

public:
    ClientSocket(const std::string host, int port);

    void connect();
};

}

#endif

