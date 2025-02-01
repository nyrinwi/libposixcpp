#include <unistd.h>
#include <sys/file.h>
#include <string>

#ifndef FILE_H
#define FILE_H

namespace posixcpp
{

class File
{
protected:
    mutable std::string m_filename;
    mutable int m_fd;
    mutable int m_mode;

    // These are the only bits preserved by the File() object
    static const int MODE_MASK = O_RDONLY|O_RDWR|O_WRONLY;

public:
    static const int PERM_GRWX = 0777;

    File() noexcept;

    File(const std::string& filename, int posixFlags, int perm=PERM_GRWX);

    File(int fd, const std::string& filename="unnamed");

    File(const File& other) noexcept;     // copy
    File& operator=(const File& other) noexcept; // copy


    File(File&& other) noexcept;    // move
    File& operator=(File&& other) noexcept; // move

    virtual ~File();

    std::string filename() const {return m_filename;};

    int fd() const {return m_fd;};

    int mode() const {return m_mode;};

    bool fdValid() const;

    bool exists() const;

    ssize_t read(void *buf, size_t count) const;

    ssize_t write(void *buf, size_t count) const;

    int close() { return ::close(m_fd); };

    off_t lseek(off_t offset, int whence=SEEK_SET) const;

    static File mkstemp(const std::string& templ);

    static File creat(const std::string& path, mode_t mode);

    // TODO  templates for read/write?
    // TODO: fcntl
};

};

#endif

