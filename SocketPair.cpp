#include "SocketPair.h"
#include "PosixError.h"

using namespace posixcpp;

SocketPair::SocketPair(int domain, int type, int protocol)
{
    int fds[2];
    int r = socketpair(domain, type, protocol, &fds[0]);
    PosixError::ASSERT(r!=-1,"socketpair");
    m_reader = File(fds[0]);
    m_writer = File(fds[1]);
}

