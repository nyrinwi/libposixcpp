#include <unistd.h>
#include <sys/file.h>
#include <string>
#include <vector>
#include <optional>

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
    std::optional<struct stat> m_stat;

    // These are the only bits preserved by the File() object
    static const int MODE_MASK = O_RDONLY|O_RDWR|O_WRONLY;

public:
    static const int PERM_GRWX = 0777;

    File() noexcept;

    File(const std::string& filename, int posixFlags=O_RDONLY, int perm=PERM_GRWX);

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

    // Requires a stat() call, does not update fstat
    bool exists() const;

    ssize_t read(void *buf, size_t count) const;

    ssize_t write(const void *buf, size_t count) const;

    int close();

    off_t lseek(off_t offset, int whence=SEEK_SET) const;
    
    void ftruncate(off_t length);

    void fsync();

    void fdatasync();

    void unlink();

    void remove();

    struct stat fstat(bool force=false);

    static File mkstemp(const std::string& templ);

    static File creat(const std::string& path, mode_t mode);

    static File mkdir(const std::string& pathname, mode_t mode);

    // TODO: fcntl

    template <typename Typ>
    ssize_t write(const Typ& data) const
    {
        size_t elemSize = sizeof(typename Typ::value_type);
        const void* buf = reinterpret_cast<const void*>(&data[0]);
        auto n = write(buf,data.size()*elemSize);
        return (n+elemSize-1)/elemSize;
    };

    template <typename Typ>
    ssize_t read(Typ& data, size_t count=0) const
    {
        size_t elemSize = sizeof(typename Typ::value_type);
        if (count == 0)
        {
            count = data.size();
        }
        else
        {
            data.resize(count);
        }
        void* buf = reinterpret_cast<void*>(&data[0]);
        ssize_t ret = 0;
        ssize_t n = read(buf,count*sizeof(typename Typ::value_type));
        if (n > 0)
        {
            ret = (n+elemSize-1)/elemSize;
            data.resize(ret);
        }
        return ret;
    };

    template <typename Typ>
    ssize_t readArray(Typ& data) const
    {
        void* buf = reinterpret_cast<void*>(&data[0]);
        return read(buf,data.size()*sizeof(typename Typ::value_type));
    };
};

};

#endif

