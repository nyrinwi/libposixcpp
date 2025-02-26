#include <fstream>
#include <array>
#include "File.h"
#include "PosixError.h"
#include <gtest/gtest.h>
#include <sys/resource.h>

using namespace posixcpp;

class FileTester : public ::testing::Test
{
    int fds[2];
    int maxFiles;
    int m_topFd = -1;
public:
    std::string m_filename;
    
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
        std::array<uint8_t,256> bytes;
        for(int i=0; i<256; i++)
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
        ::unlink(m_filename.c_str());
        for(int fd=m_topFd; fd<127; fd++)
        {
            EXPECT_EQ(-1,::fcntl(fd,F_GETFL)) << "fd: " << fd;
        }
    };
};

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
    auto fileObj = File(m_filename,O_RDWR|O_TRUNC);
    EXPECT_NO_THROW(fileObj.unlink());
    EXPECT_NO_THROW(fileObj.unlink());
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
    ::unlink(m_filename.c_str());
    File file(File::creat(m_filename,O_RDONLY));
    ASSERT_TRUE(file.exists());
}

TEST(File,mkstemp_etc)
{
    auto ff = File::mkstemp("tempfile.XXXXXX");
    ASSERT_TRUE(ff.exists());
    ASSERT_TRUE(ff.fdValid());

    // After this, file does not exist but fd is valid
    remove(ff.filename().c_str());

    ASSERT_FALSE(ff.exists());
    ASSERT_TRUE(ff.fdValid());

    ::close(ff.fd());
    ASSERT_FALSE(ff.fdValid());
}

TEST_F(FileTester,Templates)
{
    ssize_t n;
    std::vector<double> writeData{1,2,3,4,5};
    std::vector<double> readback;

    File bytes(m_filename,O_RDWR);
    bytes.ftruncate(0);
    n = bytes.write(writeData);
    ASSERT_EQ(writeData.size()*sizeof(writeData[0]),(unsigned)n);

    bytes.lseek(0);
    n = bytes.read(readback,writeData.size());
    ASSERT_EQ(writeData.size()*sizeof(writeData[0]),(unsigned)n);

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
