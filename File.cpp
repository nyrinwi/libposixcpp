#include <unistd.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <cstring>
#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include "PosixError.h"
#include "File.h"

using namespace posixcpp;

File::File(const std::string& filename, int posixFlags, int mode)
: m_filename(filename),
  m_fd(open(filename.c_str(),posixFlags,mode)),
  m_mode(::fcntl(m_fd,F_GETFL)&MODE_MASK)
{
    if (m_fd == -1)
    {
        throw PosixError(filename);
    }
}

File::File(int fd, const std::string& filename)
: m_filename(filename),
  m_fd(fd),
  m_mode(::fcntl(m_fd,F_GETFL)&MODE_MASK)
{
    PosixError::ASSERT(fd >= 0,"File(fd)");
}

File::File() noexcept
: m_fd(-1),
  m_mode(0)
{
}

File::~File()
{
    ::close(m_fd);
}

File::File(const File& other) noexcept // copy
: m_filename(other.m_filename),
  m_fd(dup(other.m_fd)),
  m_mode(::fcntl(m_fd,F_GETFL)&MODE_MASK)
{
}

File& File::operator=(const File& other) noexcept // copy
{
    if (this == &other)
    {
        return *this;
    }
    ::close(m_fd);
    m_filename = other.m_filename;
    m_fd = other.m_fd;
    m_mode = ::fcntl(m_fd,F_GETFL)&MODE_MASK;
    return *this;
}

File::File(File&& other) noexcept // move
: m_filename(other.m_filename),
  m_fd(other.m_fd),
  m_mode(::fcntl(m_fd,F_GETFL)&MODE_MASK)
{
    other.m_fd = -1;
    other.m_filename.clear();
    other.m_mode = 0;
}

File& File::operator=(File&& other) noexcept // move
{
    if (this == &other)
    {
        return *this;
    }

    ::close(m_fd);
    m_filename = other.m_filename;
    m_fd = other.m_fd;
    m_mode = ::fcntl(m_fd,F_GETFL)&MODE_MASK;
    other.m_fd = -1;
    other.m_filename.clear();
    other.m_mode = 0;
    return *this;
}

ssize_t File::read(void *buf, size_t count) const
{
    ssize_t ret = ::read(m_fd,buf,count);
    PosixError::ASSERT(ret!=-1,"read");
    return ret;
}

ssize_t File::write(const void *buf, size_t count) const
{
    ssize_t ret = ::write(m_fd,buf,count);
    PosixError::ASSERT(ret!=-1,"write");
    return ret;
}

off_t File::lseek(off_t offset, int whence) const
{
    off_t ret = ::lseek(m_fd,offset,whence);
    if (ret == -1)
    {
        std::ostringstream oss;
        oss << "lseek failed (" << m_filename << ")";
        throw PosixError(oss.str());
    }
    return ret;
}

void File::ftruncate(off_t length)
{
    int r = ::ftruncate(m_fd,length);
    PosixError::ASSERT(r!=-1,"ftruncate");
}

bool File::fdValid() const
{
    int r = ::fcntl(m_fd,F_GETFL);
    return r!=-1;
}

bool File::exists() const
{
    struct stat sbuf;
    int r = ::stat(m_filename.c_str(),&sbuf);
    return r==0;
}

File File::mkstemp(const std::string& templ)
{
    char buf[templ.size()+1];
    snprintf(&buf[0],sizeof(buf),"%s",templ.c_str());
    int fd = ::mkstemp(&buf[0]);
    PosixError::ASSERT(fd!=-1,"mkstemp");
    return File(fd,std::string(&buf[0]));
}

File File::creat(const std::string& path, mode_t mode)
{
    int fd = ::creat(path.c_str(),mode);
    return File(fd,path);
}

