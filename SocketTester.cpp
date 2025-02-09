#include "Socket.h"
#include <gtest/gtest.h>

using namespace posixcpp;
#define INTPAIR(x) x,#x

TEST(Socket,basic)
{
    std::map<int,const std::string> sockTypes = {
        {INTPAIR(SOCK_STREAM)},
        {INTPAIR(SOCK_DGRAM)},
        {INTPAIR(SOCK_RAW)},
        };

    std::map<int,const std::string> afTypes = {
        {INTPAIR(AF_UNIX)},
        {INTPAIR(AF_LOCAL)},
        {INTPAIR(AF_INET)},
        {INTPAIR(AF_BLUETOOTH)},
        };

    for (auto afType : afTypes)
    {
        for (auto sockType : sockTypes)
        {
            std::ostringstream oss;
            oss << afType.second << "," << sockType.second;
            // SOCK_RAW requires root privilege except if AF_UNIX
            if (sockType.first == SOCK_RAW and afType.first != AF_UNIX)
            {
                EXPECT_THROW(Socket sock(afType.first,sockType.first,0),PosixError) << oss.str();
                continue;
            }
            EXPECT_NO_THROW(Socket sock(afType.first,sockType.first,0)) << oss.str();
        }
    }
}

TEST(Socket,shutdown)
{
    Socket sock(AF_INET,SOCK_STREAM);
    EXPECT_NO_THROW(sock.shutdown(SHUT_WR|SHUT_RD)); // ENOTCONN is okay here
    EXPECT_FALSE(sock.shutdown(SHUT_WR|SHUT_RD)); // False tells us we had ENOTCONN
    EXPECT_THROW(sock.shutdown(9999),PosixError); // should be EINVAL
}
