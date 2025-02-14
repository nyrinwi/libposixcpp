#include <sys/socket.h>
#include <variant>
#include "Socket.h"

using namespace posixcpp;

ClientSocket::ClientSocket(const std::string host, int port)
: Socket(SOCK_STREAM, AF_INET)
{
}

void ClientSocket::connect()
{
    // FIXME - work in progress
    // sockaddr_in
    // sockaddr_in6
    // int r = connect(fd(),
}

