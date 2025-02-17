#include "Socket.h"
#include <string>
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

TEST(Socket,getaddrinfo)
{
    gai_vec_t found = Socket(AF_INET,SOCK_DGRAM).getaddrinfo("localhost");
    ASSERT_EQ(1U,found.size());
    ASSERT_EQ("127.0.0.1",found[0].first);
    ASSERT_EQ(AF_INET,found[0].second.ss_family);

    int eaiVal=0;

    found = Socket(AF_INET6,SOCK_DGRAM).getaddrinfo("localhost",&eaiVal);
    ASSERT_EQ(0U,found.size());
    ASSERT_NE(0,eaiVal);

    found = Socket(AF_INET6,SOCK_DGRAM).getaddrinfo("ip6-localnet");
    ASSERT_EQ(1U,found.size());
    ASSERT_NE(0U,found[0].first.size());
    ASSERT_EQ(AF_INET6,found[0].second.ss_family);
    ASSERT_NE("127.0.0.1",found[0].first);

    ASSERT_NO_THROW(found = Socket(AF_INET,SOCK_DGRAM).getaddrinfo("lalkjasf"));
}

TEST(ClientSocket,basic)
{
    auto found = Socket(AF_INET,SOCK_STREAM).getaddrinfo("localhost");
    ASSERT_NE(0U,found.size());
    ClientSocket<AF_INET,SOCK_STREAM> client("localhost",22);
    client.connect();
}

