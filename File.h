#include <unistd.h>
#include <sys/file.h>
#include <string>
#include <vector>
#include <optional>

#ifndef FILE_H
#define FILE_H

namespace posixcpp
{

/*
** A class wrapper for POSIX file related functions
** All functions and constructors throw PosixError on error, except where specified
*/
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

    /// Calls open() on the given filename. Throws PosixError if open returns -1.
    File(const std::string& filename, int posixFlags=O_RDONLY, int perm=PERM_GRWX);

    /// Calls fcntl(F_GETFL) on the fd and throws PosixError if fcntl returns -1.
    /// Destructor will call close() on the input file descriptor
    File(int fd, const std::string& filename="unnamed");

    // \todo make dup available?

    File(const File& other) noexcept;     // copy
    File& operator=(const File& other) noexcept; // copy

    File(File&& other) noexcept;    // move
    File& operator=(File&& other) noexcept; // move

    virtual ~File();

    /// Returns the filename given in the constructor
    std::string filename() const {return m_filename;};

    /// Returns the one and only file descriptor associated with this object
    int fd() const {return m_fd;};

    /// Returns the mode flags (RWX) assocated with the file descriptor
    int mode() const {return m_mode;};

    /// Returns true if the objects file descriptor is valid
    bool fdValid() const;

    /// Requires a stat(2) call, does not update fstat
    bool exists() const;

    /// Wrapper for read(2)
    ssize_t read(void *buf, size_t count) const;

    /// Wrapper for write(2)
    ssize_t write(const void *buf, size_t count) const;

    /// Wrapper for close(2). DOES NOT THROW
    int close() noexcept;

    /// Wrapper for lseek(2)
    off_t lseek(off_t offset, int whence=SEEK_SET) const;
    
    /// Wrapper for ftruncate(2)
    void ftruncate(off_t length);

    /// Wrapper for fsync(2)
    void fsync();

    /// Wrapper for fdatasync(2)
    void fdatasync();

    /// Wrapper for unlink(2). Does not throw if errno == ENOENT
    void unlink();

    /// Wrapper for remove(3). Does not throw if errno == ENOENT
    void remove();

    /**
     ** The underlying fstat call occurs on the first call to fstat
     ** and is never called again over the life of the object.
     */
    /// Returns a cached value of fstat on the given fd. Use force=true to refresh the cached value
    struct stat fstat(bool force=false);

    struct stat fstat() const;

    /// Wrapper for mkstemp(3)
    static File mkstemp(const std::string& templ);

    /// Wrapper for creat(2)
    static File creat(const std::string& path, mode_t mode);

    /// Wrapper for mkdir(2)
    static File mkdir(const std::string& pathname, mode_t mode);

    // \todo fcntl

    /// Write using a std::vector or std::string
    template <typename Typ>
    ssize_t write(const Typ& data) const
    {
        size_t elemSize = sizeof(typename Typ::value_type);
        const void* buf = reinterpret_cast<const void*>(&data[0]);
        auto n = write(buf,data.size()*elemSize);
        return (n+elemSize-1)/elemSize;
    };

    /// Read using a std::vector or std::string
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

    // Read into a std::array
    template <typename Typ>
    ssize_t readArray(Typ& data) const
    {
        void* buf = reinterpret_cast<void*>(&data[0]);
        return read(buf,data.size()*sizeof(typename Typ::value_type));
    };

    /// Return the size of the file as reported by fstat()
    size_t getSize(bool useCached=true);

    /// Return true if fd is a block device. Uses cached fstat
    bool is_block_device() const;

    /// Return true if fd is a char device. Uses cached fstat
    bool is_char_device() const;

    /// Return true if fd is a directory. Uses cached fstat
    bool is_dir() const;

    /// Return true if fd is a FIFO. Uses cached fstat
    bool is_fifo() const;

    /// Return true if fd is a regular file. Uses cached fstat
    bool is_file() const;

    /// Return true if fd is a mount point. Uses cached fstat
    bool is_mount() const;

    /// Return true if fd is a socket. Uses cached fstat
    bool is_socket() const;

    /// Return true if fd is a symbolic link. Uses cached fstat
    bool is_symlink() const;
};

};

#endif

