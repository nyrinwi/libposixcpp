#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include "File.h"
#include "PosixError.h"

namespace posixcpp
{
class Socket
{
    File m_file;
public:
    Socket(int domain, int type, int protocol=0);

    // Returns true if no errors, false if ENOTCONN error, throws PosixError otherwise
    bool shutdown(int how);

    int fd() const {return m_file.fd();};
};

// FIXME - work on this
class ClientSocket : public Socket
{
public:
    ClientSocket(const std::string host, int port);
};

}

#endif

