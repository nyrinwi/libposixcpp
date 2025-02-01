#ifndef SOCKETPAIR_H
#define SOCKETPAIR_H

#include <sys/types.h>
#include <sys/socket.h>
#include "File.h"

namespace posixcpp
{

class SocketPair
{
protected:
    File m_reader;
    File m_writer;

public:
    SocketPair()
    {
    };

    SocketPair(int domain, int type, int protocol=0);

    SocketPair(SocketPair&& other) noexcept    // move
    {
        m_reader = std::move(other.m_reader);
        m_writer = std::move(other.m_writer);
    };

    SocketPair& operator=(SocketPair&& other) noexcept // move
    {
        if (this != &other)
        {
            m_reader = std::move(other.m_reader);
            m_writer = std::move(other.m_writer);
        }
        return *this;
    };

    File& reader()
    {
        return m_reader;
    };

    File& writer()
    {
        return m_writer;
    };

};

}
#endif

