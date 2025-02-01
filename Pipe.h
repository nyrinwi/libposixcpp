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
};

};

#endif

