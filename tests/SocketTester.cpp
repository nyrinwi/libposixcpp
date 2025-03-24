#include "Socket.h"
#include <string>
#include <gtest/gtest.h>
#include <netdb.h>

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

TEST(Socket,getaddrinfo_static)
{
    gai_vec_t found = Socket::getaddrinfo("localhost",AF_INET);
    ASSERT_EQ(1U,found.size());
    ASSERT_EQ("127.0.0.1",found[0].first);
    ASSERT_EQ(AF_INET,found[0].second.ss_family);

    found = Socket::getaddrinfo("::1",AF_INET6);
    ASSERT_EQ(1U,found.size());

    for (int family : {AF_LOCAL,AF_INET6})
    {
        int eaiVal = -1;
        found = Socket::getaddrinfo("localhost",family,&eaiVal);
        ASSERT_EQ(0U,found.size());
        ASSERT_NE(EAI_ADDRFAMILY,eaiVal);
    }
}

class ClientSocketTester : public ::testing::Test
{
public:
    static const std::string s_message;
    static const int sinkPort = 5000;
    static const int sourcePort = 5001;
    FILE* fpTcpSink;
    FILE* fpTcpSource;
    FILE* fpUdpSink;
    FILE* fpUdpSource;

    enum NetcatType {UDP,TCP};

    FILE* socat(const NetcatType& nc, int port, bool isSource)
    {
        std::string protocol = (nc==UDP) ? "UDP" : "TCP";
        std::ostringstream oss;
        oss << "socat ";
        if (isSource)
        {
            oss << "STDIN " << protocol << "-LISTEN:" << port;
        }
        else
        {
            oss << protocol << "-LISTEN:" << port << " STDOUT";
        }
        FILE* ret = popen(oss.str().c_str(),(isSource)?"w":"r");
        if (ret == NULL)
        {
            oss << " flag " << ((isSource)?"w":"b");
            throw std::runtime_error(oss.str());
        }
        return ret;
    }

    //FILE* socat(const NetcatType& nc, int port, bool isSource)
    void SetUp() {
        fpTcpSink   = socat(TCP,sinkPort,false);
        fpTcpSource = socat(TCP,sourcePort,true);
        fpUdpSink   = socat(UDP,sinkPort,false);
        fpUdpSource = socat(UDP,sourcePort,true);
        usleep(10000);
    };

    void writeSource()
    {
        assert(fpTcpSource != NULL);
        auto n = fwrite(&s_message[0],1,s_message.size(),fpTcpSource);
        assert(n>=0);
        fflush(fpTcpSource);
    }

    std::string readSink(FILE* fp)
    {
        std::vector<char> ret(256);
        auto n = fread(&ret[0],1,ret.size(),fp);
        assert(n >= 0);
        ret.resize(n);
        return std::string(&ret[0],n);
    }

    bool waitForSink(FILE* &fp)
    {
        if (fp == NULL)
        {
            return true;
        }
        int status = pclose(fp);
        fp = NULL;
        bool ret = (status == 0);
        if (not ret)
        {
            bool signaled = WIFSIGNALED(status);
            bool exited = WIFEXITED(status);
            if (exited)
            {
                status = WEXITSTATUS(status);
            }
            else if (signaled)
            {
                status = -WTERMSIG(status);
            }
            else
            {
                status = -1;
            }
            //std::cerr << "warning error from sink "
            //    << "e:" << exited << "," << "s:" << signaled << " " << status << std::endl;
        }
        return ret;
    }

    void TearDown() {
        system("pkill -INT socat"); // TODO: smarter
        waitForSink(fpTcpSink);
        waitForSink(fpUdpSink);
        for (auto fp : {fpTcpSink,fpUdpSink,fpTcpSource,fpUdpSource})
        {
            if (fp)
            {
                pclose(fp);
            }
        }
    }
};

const std::string ClientSocketTester::s_message = "Hello World";

const int ClientSocketTester::sourcePort;

TEST_F(ClientSocketTester,read)
{
    // Give the source some data
    writeSource();

    // Connect to source
    ClientSocket<AF_INET,SOCK_STREAM> client("localhost",sourcePort);
    ASSERT_NO_THROW(client.connect()) << "failed to connect TCP to " << sourcePort;

    // Read from source
    std::array<char,256> buf;
    auto r = client.read(&buf[0],s_message.size());
    ASSERT_EQ((unsigned)r,s_message.size());

    // Verify we got s_message
    std::string resp(&buf[0],r);
    ASSERT_EQ(resp,s_message);
}

TEST_F(ClientSocketTester,recv)
{
    // Give the source some data
    writeSource();

    // Connect to source
    ClientSocket<AF_INET,SOCK_STREAM> client("localhost",sourcePort);
    ASSERT_NO_THROW(client.connect());

    // Read from source
    std::array<char,256> buf;
    auto r = client.recv(&buf[0],s_message.size());
    ASSERT_EQ((unsigned)r,s_message.size());

    // Verify we got s_message
    ASSERT_EQ((unsigned)r,s_message.size());
    client.close();
}

TEST_F(ClientSocketTester,write)
{
    // Connect to sink
    ClientSocket<AF_INET,SOCK_STREAM> client("localhost",sinkPort);
    ASSERT_NO_THROW(client.connect());

    // Write to the server
    auto r = client.write(&s_message[0],s_message.size());
    ASSERT_EQ(s_message.size(),(unsigned)r);

    client.close();

    auto text = readSink(fpTcpSink);
    ASSERT_EQ(s_message,text);
}

TEST_F(ClientSocketTester,send)
{
    // Connect to sink
    ClientSocket<AF_INET,SOCK_STREAM> client("localhost",sinkPort);
    ASSERT_NO_THROW(client.connect());

    // Write to the server
    auto r = client.send(&s_message[0],s_message.size());
    ASSERT_EQ(s_message.size(),(unsigned)r);

    client.close();

    auto text = readSink(fpTcpSink);
    ASSERT_EQ(s_message,text);
}

TEST_F(ClientSocketTester,sendto)
{
    Socket sock(AF_INET,SOCK_DGRAM);
    auto found = sock.getaddrinfo("localhost");
    ASSERT_EQ(1U,found.size());

    auto sa = reinterpret_cast<sockaddr*>(&found[0].second);
    auto sai = reinterpret_cast<sockaddr_in*>(&found[0].second);
    sai->sin_port = htons(sinkPort);

    auto r = sock.sendto(&s_message[0],s_message.size(),0, sa, sizeof(*sai));
    ASSERT_EQ(s_message.size(),(unsigned)r) << strerror(errno) << " fd=" << sock.fd();
}

TEST_F(ClientSocketTester,badconn)
{
    // Attempt to connect to non-existent host
    ClientSocket<AF_INET,SOCK_STREAM> client("nosuhchost",sinkPort);
    ASSERT_THROW(client.connect(),PosixError);
}

