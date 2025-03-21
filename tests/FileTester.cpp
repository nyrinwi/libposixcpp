#include <fstream>
#include <array>
#include <functional>
#include "File.h"
#include "PosixError.h"
#include <gtest/gtest.h>
#include <sys/resource.h>
#include <sys/socket.h>

using namespace posixcpp;

class FileTester : public ::testing::Test
{
    int fds[2];
    int maxFiles;
    int m_topFd = -1;
public:
    std::string m_filename;
    std::string m_dirname;
    static constexpr size_t c_fileSize = 256;
    
    int readFd() {return fds[0];};
    int writeFd() {return fds[1];};

    void SetUp()
    {
        struct rlimit limit;
        int r = getrlimit(RLIMIT_NOFILE,&limit);
        PosixError::ASSERT(r==0,"getrlimit");
        maxFiles = limit.rlim_cur;

        // Create a pipe for testing
        r = pipe(&fds[0]);
        PosixError::ASSERT(r==0,"pipe");

        // Fill m_filename file with sequential bytes
        m_filename = "test.dat";
        m_dirname = "test.dir";
        std::array<uint8_t,c_fileSize> bytes;
        for(unsigned i=0; i<c_fileSize; i++)
        {
            bytes[i] = i;
        }

        std::ofstream fp(m_filename);
        fp.write((char*)&bytes[0],bytes.size());
        fp.close();
        for (int fd=3; fd<20; fd++)
        {
            if (::fcntl(fd,F_GETFL)==-1)
            {
                m_topFd = fd;
                break;
            }
        }
    };

    void TearDown()
    {
        ::close(fds[0]);
        ::close(fds[1]);
        for (auto path : {m_filename,m_dirname})
        {
            try
            {
                File(path).remove();
            }
            catch(const PosixError& e)
            {
                if (e.errnoVal() != ENOENT && e.errnoVal() != EBADF)
                {
                    FAIL() << e.what();
                }
            }
        }
        for(int fd=m_topFd; fd<127; fd++)
        {
            EXPECT_EQ(-1,::fcntl(fd,F_GETFL)) << "fd: " << fd;
        }
    };

    void testMutuallyExlusiveIsMethods(const File &file);
};

/// Verify all mutually exclusive is_xyz() methods are mutually exlusive
void FileTester::testMutuallyExlusiveIsMethods(const File &file)
{
    std::vector<bool> values;
    values.push_back(std::bind(&File::is_block_device,&file)());
    values.push_back(std::bind(&File::is_char_device,&file)());
    values.push_back(std::bind(&File::is_dir,&file)());
    values.push_back(std::bind(&File::is_fifo,&file)());
    values.push_back(std::bind(&File::is_file,&file)());
    values.push_back(std::bind(&File::is_socket,&file)());
    // values.push_back(std::bind(&File::is_symlink,&file)());
    unsigned count = std::count(values.begin(),values.end(),true);
    ASSERT_EQ(1U,count);
}

std::string whatIsIt(File& file)
{
    std::map<const char*,std::function<bool()>> methods {
        {"is_block_device",std::bind(&File::is_block_device,&file)},
        {"is_char_device",std::bind(&File::is_char_device,&file)},
        {"is_dir",std::bind(&File::is_dir,&file)},
        {"is_fifo",std::bind(&File::is_fifo,&file)},
        {"is_file",std::bind(&File::is_file,&file)},
        {"is_socket",std::bind(&File::is_socket,&file)},
        {"is_symlink",std::bind(&File::is_symlink,&file)},
    };
    std::ostringstream oss;
    for (auto pair : methods)
    {
        if (not pair.second())
        {
            continue;
        }

        if (not oss.str().empty())
        {
            oss << ",";
        }
        oss << pair.first;
    }
    return oss.str();
}

TEST_F(FileTester,basic)
{
    EXPECT_THROW(File file("lajsdffljas",O_RDONLY),PosixError);

    for (auto mode : {O_RDONLY, O_WRONLY, O_RDWR})
    {
        ASSERT_NO_THROW(File file(m_filename,mode)) << "mode: " << mode;
        File file(m_filename,mode);
        ASSERT_EQ(mode,file.mode());
    }
    auto fileObj = File(m_filename,O_RDONLY);
    fileObj.close();
    EXPECT_NO_THROW(fileObj.close());
}

TEST_F(FileTester,fsync)
{
    auto fileObj = File(m_filename,O_RDWR|O_TRUNC);
    EXPECT_NO_THROW(fileObj.fsync());
}

TEST_F(FileTester,fdatasync)
{
    auto fileObj = File(m_filename,O_RDWR|O_TRUNC);
    EXPECT_NO_THROW(fileObj.fdatasync());
}

TEST_F(FileTester,unlink)
{
    auto file = File(m_filename);
    ASSERT_TRUE(file.exists());
    EXPECT_NO_THROW(file.unlink());
    ASSERT_FALSE(file.exists());
    EXPECT_NO_THROW(file.unlink());

    auto dir = File::mkdir(m_dirname,0777);
    ASSERT_TRUE(dir.exists());
    EXPECT_THROW(dir.unlink(),PosixError) << "cannot use unlink on a dir";
    ASSERT_TRUE(dir.exists());
    file = File::creat(m_dirname+"/foo.dat",S_IRWXU);
    file.remove();
}

TEST_F(FileTester,withFd)
{
    // Use the pipe as our fd
    File reader(readFd());
    File writer(writeFd());
    std::array<char,6> msg{"hello"};
    auto size = writer.write(&msg[0],msg.size());
    ASSERT_EQ(size,ssize_t(sizeof(msg)));

    std::array<char,6> recvBuf;
    size = reader.read(&recvBuf[0],msg.size());
    ASSERT_EQ(size,ssize_t(msg.size()));
    ASSERT_EQ(msg,recvBuf);
}

TEST_F(FileTester,read_template)
{
    std::vector<char> recvBuf(100);
    File reader("/dev/zero");
    auto size = reader.read(recvBuf);
    ASSERT_EQ(recvBuf.size(),(unsigned)size);
}

TEST_F(FileTester,lseek)
{
    int seekLoc = 128;
    File bytes(m_filename,O_RDONLY);
    auto nn = bytes.lseek(seekLoc);
    ASSERT_EQ(nn,seekLoc);
    nn = bytes.lseek(0,SEEK_CUR);
    ASSERT_EQ(nn,seekLoc);

    // sunny day case...
    char cc;
    bytes.read(&cc,1);
    ASSERT_EQ(char(seekLoc),cc);

    bytes.read(&cc,1);
    ASSERT_EQ(char(seekLoc+1),cc);

    // Rainy day case
    File pipeReader(readFd());

    // Cannot seek on a pipe - this should throw
    off_t where = 0x1;
    EXPECT_THROW(pipeReader.lseek(where,SEEK_SET),PosixError);
}

TEST_F(FileTester,move1)
{
    // Calls File::File(File&& other)
    auto aaa = File(m_filename,O_RDWR);
    int fd = aaa.fd();
    File bbb = std::move(aaa);
    ASSERT_EQ(-1,aaa.fd()) << "moved fd should be -1";
    ASSERT_EQ(fd,bbb.fd());
}

TEST_F(FileTester,move2)
{
    // Calls File::operator=(File&& other)
    std::string& filename = m_filename;
    std::vector<File> foo(3);
    std::generate(foo.begin(),foo.end(),[filename](){return File(filename,O_RDONLY);});
    foo[0] = foo.back();
    for(auto& ff : foo)
    {
        std::cout << (&ff-&foo[0]) << " : fd=" << ff.fd() << std::endl;
    }
    foo.clear();
}

TEST_F(FileTester,fd_and_copy)
{
    // fds increment by one with each open call
    File file1(m_filename,O_RDONLY);
    File file2(m_filename,O_RDONLY);
    ASSERT_GT(file2.fd(),file1.fd());
    ASSERT_EQ(1,file2.fd()-file1.fd());

    // Copy
    File file3 = file2;
    ASSERT_EQ(1,file3.fd()-file2.fd()) << "copy should get a new fd";
    ASSERT_EQ(file3.filename(),file2.filename());

    // Both fds are "entangled" by dup()
    auto nn = file2.lseek(100);
    ASSERT_EQ(100,nn);
    auto mm = file3.lseek(0,SEEK_CUR);
    ASSERT_EQ(100,mm);
}

TEST_F(FileTester,creat)
{
    ASSERT_NO_THROW(File(m_filename).remove());
    auto file = File::creat(m_filename,S_IRWXU);
    ASSERT_TRUE(file.exists());
    ASSERT_TRUE(file.fdValid());
}

TEST_F(FileTester,mkdir)
{
    File dir(File::mkdir(m_dirname,0555));
    ASSERT_TRUE(dir.exists());
    dir.remove();
    ASSERT_FALSE(dir.exists());
    EXPECT_NO_THROW(dir.remove());
}

TEST_F(FileTester,getcwd)
{
    std::string cwd;
    ASSERT_NO_THROW(cwd = File::getcwd());
    ASSERT_TRUE(cwd[0] = '/') << cwd;
}

TEST_F(FileTester,normalizePath)
{
    EXPECT_EQ(File::normalizePath(""),"");

    ASSERT_EQ(File::normalizePath("./"),".");
    ASSERT_EQ(File::normalizePath(".."),"./..");
    ASSERT_EQ(File::normalizePath("../"),"./..");
    ASSERT_EQ(File::normalizePath(".//"),".");
    EXPECT_EQ(File::normalizePath("a"),"./a");
    EXPECT_EQ(File::normalizePath("a/"),"./a");
    EXPECT_EQ(File::normalizePath("a//"),"./a");
    EXPECT_EQ(File::normalizePath("a/b"),"./a/b");
    EXPECT_EQ(File::normalizePath("a//b"),"./a/b");
    EXPECT_EQ(File::normalizePath("a//b/"),"./a/b");
    EXPECT_EQ(File::normalizePath("a/./b/"),"./a/./b");

    EXPECT_EQ(File::normalizePath("/"),"/");
    EXPECT_EQ(File::normalizePath("//"),"/");
    EXPECT_EQ(File::normalizePath("///"),"/");
    EXPECT_EQ(File::normalizePath("/a"),"/a");
    EXPECT_EQ(File::normalizePath("//a"),"/a");
    EXPECT_EQ(File::normalizePath("/a/"),"/a");
    EXPECT_EQ(File::normalizePath("//a/b"),"/a/b");
}

TEST_F(FileTester,remove)
{
    File file(m_filename);
    ASSERT_NO_THROW(file.remove());
    ASSERT_NO_THROW(file.remove());
}

TEST_F(FileTester,fstat)
{
    auto file = File(m_filename);
    auto sbuf = file.fstat();
    auto sbuf1 = file.fstat(); //for coverage
    ASSERT_EQ(sbuf.st_mode,sbuf1.st_mode);
    ASSERT_TRUE(S_ISREG(sbuf.st_mode));
    file.close();
    EXPECT_NO_THROW(file.fstat()) << "fstat remains until forced";
    EXPECT_THROW(file.fstat(true),PosixError) << "force fstat on bad file descr";
    EXPECT_THROW(file.fstat(),PosixError) << "fstat should have been cleared";
}

TEST(File,mkstemp_etc)
{
    auto ff = File::mkstemp("tempfile.XXXXXX");
    ASSERT_TRUE(ff.exists());
    ASSERT_TRUE(ff.fdValid());

    // After this, file does not exist but fd is valid
    ff.remove();

    ASSERT_FALSE(ff.exists());
    ASSERT_TRUE(ff.fdValid());

    ::close(ff.fd());
    ASSERT_FALSE(ff.fdValid());
}

TEST_F(FileTester,templates)
{
    ssize_t n;
    std::vector<double> writeData{1,2,3,4,5};
    std::vector<double> readback;

    File bytes(m_filename,O_RDWR|O_TRUNC);
    n = bytes.write(writeData);
    ASSERT_EQ(writeData.size(),(unsigned)n);
    bytes.fsync();

    bytes.lseek(0);

    n = bytes.read(readback,writeData.size());
    ASSERT_EQ(writeData.size(),(unsigned)n);

    ASSERT_EQ(writeData,readback);

    n = bytes.read(readback,writeData.size());
    ASSERT_EQ(0,n);
}

TEST_F(FileTester,array)
{
    ssize_t n;
    std::array<double,5> writeData{1,2,3,4,5};
    std::array<double,5> readback;

    File bytes(m_filename,O_RDWR);
    bytes.ftruncate(0);

    // write() can be usd with array
    n = bytes.write(writeData);

    bytes.lseek(0);

    // array read requires readArray()
    n = bytes.readArray(readback);
    ASSERT_EQ(writeData.size()*sizeof(writeData[0]),(unsigned)n);
    for(unsigned i=0; i<writeData.size(); i++)
    {
        ASSERT_EQ(writeData[i],readback[i]);
    }
}

TEST_F(FileTester,strings)
{
    std::string msg = "Hello World";
    File file(m_filename,O_RDWR);
    file.ftruncate(0);
    file.write(msg);

    file.lseek(0);

    // Why does this work?
    std::string response;
    file.read(response,msg.size());
    ASSERT_NE(0U,msg.size());
    ASSERT_EQ(msg,std::string(response.begin(),response.end()));
    ASSERT_EQ(msg,response);
}

TEST_F(FileTester,std_string)
{
    std::string buf{__PRETTY_FUNCTION__};
    File file(m_filename,O_RDWR|O_TRUNC);
    file.write(buf);
    file.close();


    // \todo more tests, ditto for std::vector

    std::string readback;
    file = File(m_filename,O_RDONLY);
    file.read(readback,buf.size());
    ASSERT_EQ(buf,readback);
}

TEST_F(FileTester,pathValidated)
{
    File devNull("/dev/null");
    ASSERT_TRUE(devNull.pathValidated());

    File fd(devNull.fd());
    ASSERT_FALSE(fd.pathValidated());
}

TEST_F(FileTester,getSize)
{
    File file(m_filename);
    ASSERT_EQ(c_fileSize,file.getSize());

    // Truncate the file
    File(m_filename,O_WRONLY|O_TRUNC);
    ASSERT_EQ(c_fileSize,file.getSize()) << "default should be to use cached value";
    ASSERT_EQ(c_fileSize,file.getSize(true)) << "true should be the default";
    ASSERT_EQ(0U,file.getSize(false)) << "did not use cached value";

    // Use a pipe() which should have zero size
    int fds[2];
    ASSERT_EQ(0,pipe(fds));
    File readFd(fds[0]);
    File writeFd(fds[1]);
    ASSERT_EQ(0U,readFd.getSize());
    ASSERT_EQ(0U,writeFd.getSize());
}

TEST_F(FileTester,is_file)
{
    auto file = File(m_filename,O_RDONLY);
    EXPECT_TRUE(file.is_file());
    testMutuallyExlusiveIsMethods(file);
}

TEST_F(FileTester,is_mount)
{
    ASSERT_TRUE(File("/dev/shm").is_mount());
    ASSERT_TRUE(File("/dev").is_mount());
    ASSERT_TRUE(File("/proc").is_mount());
    ASSERT_TRUE(File("/").is_mount());
    ASSERT_FALSE(File("/tmp").is_mount());
    ASSERT_FALSE(File(".").is_mount());
    ASSERT_FALSE(File(m_filename).is_mount());
}

TEST_F(FileTester,is_socket)
{
    auto file = File(socket(AF_INET,SOCK_STREAM,0));
    EXPECT_TRUE(file.is_socket());
    testMutuallyExlusiveIsMethods(file);
}

TEST_F(FileTester,is_fifo)
{
    int fds[2];
    ASSERT_EQ(0,pipe(fds));
    auto file = File(fds[0]);
    EXPECT_TRUE(file.is_fifo());
    testMutuallyExlusiveIsMethods(file);

    file = File(fds[1]);
    EXPECT_TRUE(file.is_fifo());
    testMutuallyExlusiveIsMethods(file);

}

// \todo TEST_F(FileTester,is_block_device)

TEST_F(FileTester,is_char_device)
{
    auto file = File("/dev/null",O_RDONLY);
    EXPECT_TRUE(file.is_char_device());
    testMutuallyExlusiveIsMethods(file);
}

TEST_F(FileTester,is_dir)
{
    auto file = File(".",O_RDONLY);
    EXPECT_TRUE(file.is_dir());
    testMutuallyExlusiveIsMethods(file);
}

TEST_F(FileTester,is_symlink)
{
    auto procSelf = File("/proc/self",O_RDONLY);
    EXPECT_TRUE(procSelf.is_dir()) << whatIsIt(procSelf);
    ASSERT_TRUE(procSelf.is_symlink()) << whatIsIt(procSelf);
    testMutuallyExlusiveIsMethods(procSelf);

    File fd(open("/dev/null",O_RDWR));
    ASSERT_FALSE(fd.is_symlink()) << "fd cannot be a symlink";

    auto file = File(m_filename);
    ASSERT_FALSE(file.is_symlink());
    file.remove();
    ASSERT_FALSE(file.is_symlink());
}

TEST_F(FileTester,parent)
{
    File parent = File(m_filename).parent().filename();
    ASSERT_EQ(".",parent.filename()) << parent;

    std::string cwd = File::getcwd();

    parent = parent.parent();
    ASSERT_NE(".",parent.filename()) << parent;
    ASSERT_EQ(0U,cwd.find(parent.filename())) << parent;

    File dev("/dev");
    ASSERT_NO_THROW(dev.parent()) << dev;
    ASSERT_NO_THROW(dev.parent().parent());

    File fd(open("/dev/null",O_RDWR));
    ASSERT_THROW(fd.parent(),PosixError);

    // parent of / is /
    File root("/");
    ASSERT_TRUE(root==root.parent()) << root << ", " << root.parent();
}
