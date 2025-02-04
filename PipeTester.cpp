#include <array>
#include <list>
#include "Pipe.h"
#include "PosixError.h"
#include <gtest/gtest.h>
#include <sys/resource.h>
#include <sys/errno.h>

using namespace posixcpp;

class PipeTester : public ::testing::Test
{
protected:
    unsigned maxFiles;

    void SetUp() {
        struct rlimit limit;
        int r = getrlimit(RLIMIT_NOFILE,&limit);
        PosixError::ASSERT(r==0,"getrlimit");
        maxFiles = limit.rlim_cur;
    };
};

TEST_F(PipeTester,basic)
{
    auto pipe = Pipe();
    std::array<char,12> message{"Hello World"};
    std::array<char,12> response;

    pipe.writer().write(message);
    pipe.reader().readArray(response);
    ASSERT_EQ(message,response);
}

TEST_F(PipeTester,fromFd)
{
    int fds[2];
    int r = pipe(&fds[0]);
    ASSERT_EQ(0,r) << "internal error";
    Pipe pipe(fds[0],fds[1]);
    ASSERT_TRUE(pipe.reader().fdValid());
    ASSERT_TRUE(pipe.writer().fdValid());

}

TEST_F(PipeTester,rainyDay)
{
    std::list<Pipe> pipes;
    for(unsigned i=0; i<maxFiles/2; i++)
    {
        try
        {
            pipes.push_back(Pipe());
        }
        catch(const PosixError& e)
        {
            EXPECT_EQ(EMFILE,e.errnoVal())  << i << "," << maxFiles << " exception - " << e.what() << std::endl;
            EXPECT_GT(2*i,maxFiles*3/4);
            break;
        }
    }
}

