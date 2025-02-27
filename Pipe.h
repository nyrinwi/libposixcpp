#ifndef FOO_H
#define FOO_H

#include "File.h"

namespace posixcpp
{

class Pipe
{
protected:
    File m_reader;
    File m_writer;

public:
    Pipe();
    Pipe(int readFd, int writefd);

    File& reader()
    {
        return m_reader;
    };

    File& writer()
    {
        return m_writer;
    };

    Pipe(Pipe&& other) noexcept    // move
    {
        m_reader = std::move(other.m_reader);
        m_writer = std::move(other.m_writer);
    };

    Pipe& operator=(Pipe&& other) noexcept // move
    {
        if (this != &other)
        {
            m_reader = std::move(other.m_reader);
            m_writer = std::move(other.m_writer);
        }
        return *this;
    };
};

};

#endif

