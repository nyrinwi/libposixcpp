#ifndef SOCKET_H
#define SOCKET_H

#include <variant>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "File.h"
#include "PosixError.h"

namespace posixcpp
{

typedef std::vector<std::string> stringvec_t;
typedef std::vector<std::pair<std::string,sockaddr_storage>> gai_vec_t;

class Socket
{
    File m_file;
    const int m_domain;
    const int m_type;
    const int m_protocol;
public:

    Socket(int domain, int type, int protocol=0);

    int fd() const {return m_file.fd();};

    int domain() const {return m_domain;};
 
    int protocol() const {return m_protocol;};

    // Returns true if no errors, false if ENOTCONN error, throws PosixError otherwise
    bool shutdown(int how=SHUT_RDWR);

    // Returns empty vec if failed, *eaiVal is non-zero if failed
    gai_vec_t  getaddrinfo(const std::string& host, int *eaiVal=NULL);
};

// socket
template <int Domain, int Type>
class ClientSocket : public Socket
{
protected:
    const std::string m_server;
    const int m_port;
    int m_clientFd;
public:
    ClientSocket(const std::string host, int port)
    : Socket(Domain,Type),
      m_server(host),
      m_port(port)
    {
    };

    ~ClientSocket()
    {
        shutdown();
    };

    void connect()
    {
        int eaiVal=0;
        gai_vec_t found = getaddrinfo(m_server,&eaiVal);
        if (found.size() == 0)
        {
            throw PosixError("getaddrinfo");
        }
        auto& sockAddr = found[0].second;
        ((sockaddr_in*)&sockAddr)->sin_port = htons(m_port);
        m_clientFd = ::connect(fd(),(const sockaddr*)&sockAddr,sizeof(sockAddr));
        PosixError::ASSERT(m_clientFd!=-1,"connect");
    };
};

}
#endif

