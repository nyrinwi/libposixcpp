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

class ClientSocketTester : public ::testing::Test
{
public:
    static const std::string message;
    FILE* fpNetcat;

    void SetUp() {
        fpNetcat = popen("netcat -l localhost 5000 > sink.dat","w");
        assert(fpNetcat != NULL);
        fwrite(&message[0],message.size(),1,fpNetcat);
        fflush(fpNetcat);
        usleep(10000);
    };

    std::vector<char> readSink()
    {
        std::vector<char> ret(256);
        File sink("sink.dat",O_RDONLY);
        ssize_t n = sink.read(ret);
        assert(n >= 0);
        ret.resize(n);
        return ret;
    }

    bool waitForNetcat()
    {
        auto ret = pclose(fpNetcat);
        fpNetcat = NULL;
        return ret == 0;
    }

    void TearDown() {
        if (fpNetcat)
        {
            waitForNetcat();
        }
        File file("sink.dat",O_RDWR);
        file.unlink();
    }
};

const std::string ClientSocketTester::message = "Hello World";

TEST_F(ClientSocketTester,basic)
{
    ClientSocket<AF_INET,SOCK_STREAM> client("localhost",5000);
    ASSERT_NO_THROW(client.connect());

    std::array<char,256> buf;
    auto r = client.read(&buf[0],buf.size());
    ASSERT_GT(r,0);
    std::string s(&buf[0],r);
    ASSERT_EQ(s,"Hello World");

    std::string msg("This is the response");
    r = client.write(msg);
    ASSERT_EQ((unsigned)r,msg.size());
    EXPECT_NO_THROW(client.shutdown(SHUT_WR|SHUT_RD));
    EXPECT_NO_THROW(client.close());
    waitForNetcat();

    std::vector<char> response = readSink();
    std::string responseStr(&response[0],response.size());
    ASSERT_EQ(msg,responseStr);
}

TEST_F(ClientSocketTester,send)
{
    ClientSocket<AF_INET,SOCK_STREAM> client("localhost",5000);
    ASSERT_NO_THROW(client.connect());

    std::vector<char> wtf{'w','t','f'};
    client.send(&wtf[0],wtf.size(),0);
    EXPECT_NO_THROW(client.shutdown(SHUT_WR|SHUT_RD));
    client.close();
    waitForNetcat();
    std::vector<char> response = readSink();
    ASSERT_FALSE(response.empty());
    ASSERT_EQ(wtf,response);
}

