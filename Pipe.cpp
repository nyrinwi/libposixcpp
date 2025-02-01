#include <unistd.h>
#include <sys/file.h>

#include "File.h"
#include "PosixError.h"
#include "Pipe.h"

using namespace posixcpp;

Pipe::Pipe()
{
    int fds[2];
    int r = ::pipe(fds);
    if (r == -1)
    {
        throw PosixError("pipe");
    }
    m_reader = File(fds[0]);
    m_writer = File(fds[1]);
}

Pipe::Pipe(int readFd, int writeFd)
{
    m_reader = File(readFd);
    m_writer = File(writeFd);
}
