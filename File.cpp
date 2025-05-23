#include <unistd.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <cstring>
#include <climits>
#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include "PosixError.h"
#include "File.h"

using namespace posixcpp;

File::File(const std::string& filename, int posixFlags, int mode)
: m_filename(normalizePath(filename)),
  m_fd(open(filename.c_str(),posixFlags,mode)),
  m_fromFilename(true)
{
    if (m_fd == -1)
    {
        throw PosixError(filename);
    }
    fstat();
    m_mode = ::fcntl(m_fd,F_GETFL)&MODE_MASK;
}

File::File(int fd, const std::string& filename)
: m_filename(normalizePath(filename)),
  m_fd(fd),
  m_mode(::fcntl(m_fd,F_GETFL)&MODE_MASK),
  m_stat(fstat(true)),
  m_fromFilename(false)
{
    PosixError::ASSERT(fd >= 0,"File(fd)");
}

File::File() noexcept
: m_fd(-1),
  m_mode(0),
  m_fromFilename(false)
{
}

File::~File()
{
    ::close(m_fd);
}

File::File(const File& other) noexcept // copy
: m_filename(other.m_filename),
  m_fd(::dup(other.m_fd)),
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
    m_stat = other.m_stat;
    m_fromFilename = other.m_fromFilename;
    return *this;
}

File::File(File&& other) noexcept // move
: m_filename(other.m_filename),
  m_fd(other.m_fd),
  m_mode(::fcntl(m_fd,F_GETFL)&MODE_MASK),
  m_fromFilename(other.m_fromFilename)
{
    other.m_fd = -1;
    other.m_filename.clear();
    other.m_mode = 0;
    struct stat zz{0};
    other.m_stat = zz;
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
    m_stat = other.m_stat;
    m_fromFilename = other.m_fromFilename;
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

int File::close() noexcept
{
    int fd = m_fd;
    if (fd == -1)
    {
        return 0;
    }
    m_fd = -1;
    return ::close(fd);
};

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

void File::fsync()
{
    int r = ::fsync(m_fd);
    PosixError::ASSERT(r!=-1,"fsync");
}

void File::fdatasync()
{
    int r = ::fdatasync(m_fd);
    PosixError::ASSERT(r!=-1,"fdatasync");
}

void File::unlink()
{
    int r = ::unlink(m_filename.c_str());
    if (r==-1 and errno != ENOENT)
    {
        throw PosixError("unlink failed");
    }
}

void File::remove()
{
    int r = ::remove(m_filename.c_str());
    if (r==-1 and errno != ENOENT)
    {
        throw PosixError("remove failed");
    }
}

struct stat File::fstat(bool force)
{
    if (not m_stat or force)
    {
        struct stat sbuf;
        int r = ::fstat(m_fd,&sbuf);
        if (r == 0)
        {
            m_stat = sbuf;;
        }
        else
        {
            m_stat.reset();
        }
        PosixError::ASSERT(r==0,"fstat");
    }
    return *m_stat;
}

struct stat File::fstat() const
{
    return *m_stat;
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
    PosixError::ASSERT(fd!=-1,"creat");
    return File(fd,path);
}

File File::mkdir(const std::string& path, mode_t mode)
{
    int r = ::mkdir(path.c_str(),mode);
    PosixError::ASSERT(r!=-1,"mkdir");
    return File(path,O_RDONLY);
}

std::string File::getcwd()
{
    char path[PATH_MAX+1];
    char* retPtr = ::getcwd(&path[0],PATH_MAX);
    PosixError::ASSERT(retPtr!=NULL,"getcwd");
    return retPtr;
}

File File::dup(int fd)
{
    int newFd = ::dup(fd);
    PosixError::ASSERT(newFd!=-1);
    return File(newFd);
}

std::string File::normalizePath(const std::string& path)
{
    if (path.empty())
    {
        return "";
    }
    std::istringstream iss(path);
    std::ostringstream oss;
    while (iss)
    {
        std::string tok;
        if (iss.eof())
        {
            break;
        }
        getline(iss,tok,'/');
        if (oss.str().empty())
        {
            if (tok.empty())
            {
                oss << "/";
            }
            else if (tok == ".")
            {
                oss << ".";
            }
            else
            {
                oss << "./" << tok;
            }
        }
        else if (tok.empty())
        {
            continue;
        }
        else
        {
            if (oss.str().back()!='/')
            {
                oss << '/';
            }
            oss << tok;
        }
    }
    return oss.str();
}

struct stat File::lstat(const std::string& filename)
{
    struct stat ret;
    int r = ::lstat(filename.c_str(),&ret);
    PosixError::ASSERT(r!=-1);
    return ret;
}

size_t File::getSize(bool useCached)
{
    auto stat = fstat(not useCached);
    return stat.st_size;
}

bool File::is_block_device() const
{
    return S_ISBLK((*m_stat).st_mode);
}

bool File::is_char_device() const
{
    return S_ISCHR((*m_stat).st_mode);
}

bool File::is_dir() const
{
    return S_ISDIR((*m_stat).st_mode);
}

bool File::is_fifo() const
{
    return S_ISFIFO((*m_stat).st_mode);
}

bool File::is_file() const
{
    return S_ISREG((*m_stat).st_mode);
}

bool File::is_mount() const
{
    if (not is_dir())
    {
        return false;
    }
    return m_filename == "/" or fstat().st_dev != parent().fstat().st_dev;
}

bool File::is_socket() const
{
    return S_ISSOCK((*m_stat).st_mode);
}

bool File::is_symlink() const
{
    // Note that an open file descriptor cannot be a symlink, so we must use the path
    if (not m_fromFilename)
    {
        return false;
    }
    bool ret = false;
    try
    {
        struct stat sbuf;
        sbuf = lstat(m_filename);
        ret = S_ISLNK(sbuf.st_mode);
    }
    catch(const std::exception & e)
    {
        ret = false;
    }
    return ret;
}

File File::parent() const
{
    if (not m_fromFilename)
    {
        throw PosixError("fd has no parent()",EINVAL);
    }

    std::string path;

    size_t n = m_filename.find_last_of("/");
    File ret;
    if (m_filename == ".")
    {
        /// Special case - path == "."
        ret = File(File::getcwd(),O_RDONLY).parent();
    }
    else if (n == 0 or m_filename == "/")
    {
        ret = File("/");
    }
    else
    {
        assert(n!=std::string::npos);
        std::string p = m_filename.substr(0,n);
        ret = File(m_filename.substr(0,n),O_RDONLY);
    }
    return ret;
}

std::ostream& operator<<(std::ostream& os, const File& file)
{
    os << "File(";
    if (file.pathValidated())
    {
        os << "\"" << file.filename() << "\")";
    }
    else
    {
        os << "\"" << file.fd() << "\")";
    }
    return os;
}

bool File::operator==(const File& other) const
{
    return fstat().st_ino == other.fstat().st_ino;
}
