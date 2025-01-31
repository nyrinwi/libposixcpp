#include <fstream>
#include <array>
#include "File.h"
#include "PosixError.h"
#include <gtest/gtest.h>

using namespace posixcpp;

class FileTester : public ::testing::Test
{
    int fds[2];
public:
    std::string m_bytes;
    
    int readFd() {return fds[0];};
    int writeFd() {return fds[1];};
    void SetUp()
    {
        // TODO: make this a static method or a class
        int r = pipe(&fds[0]);
        PosixError::ASSERT(r==0,"pipe");

        std::array<uint8_t,256> bytes;
        for(int i=0; i<256; i++)
        {
            bytes[i] = i;
        }
        m_bytes = "bytes.dat";
        std::ofstream fp(m_bytes);
        fp.write((char*)&bytes[0],bytes.size());
        fp.close();

    };
    void TearDown()
    {
        ::close(fds[0]);
        ::close(fds[1]);
        ::unlink(m_bytes.c_str());
    };
};

TEST_F(FileTester,basic)
{
    EXPECT_THROW(File file("lajsdffljas",O_RDONLY),PosixError);

    for (auto mode : {O_RDONLY, O_WRONLY, O_RDWR})
    {
        ASSERT_NO_THROW(File file(__FILE__,mode));
        File file(__FILE__,mode);
        ASSERT_EQ(mode,file.mode());
    }
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
    File bytes(m_bytes,O_RDONLY);
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
    auto aaa = File(m_bytes,O_RDWR);
    int fd = aaa.fd();
    File bbb = std::move(aaa);
    ASSERT_EQ(-1,aaa.fd()) << "moved fd should be -1";
    ASSERT_EQ(fd,bbb.fd());
}

TEST_F(FileTester,move2)
{
    // Calls File::operator=(File&& other)
    std::string filename = m_bytes;
    std::vector<File> foo(3);
    std::generate(foo.begin(),foo.end(),[filename](){return File(filename,O_RDONLY);});
    foo[0] = foo.back();
}

TEST_F(FileTester,fd_and_copy)
{
    // fds increment by one with each open call
    File file1(m_bytes,O_RDONLY);
    File file2(m_bytes,O_RDONLY);
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
    ::unlink(m_bytes.c_str());
    File file(File::creat(m_bytes,O_RDONLY));
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

