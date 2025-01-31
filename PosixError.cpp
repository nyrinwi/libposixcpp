#include <cstring>
#include <array>
#include <sstream>
#include "PosixError.h"

using namespace posixcpp;

PosixError::PosixError(const std::string& msg)
: m_errno(errno)
{
    std::array<char,128> msgBuf;
    const char*eMsg = strerror_r(m_errno,&msgBuf[0],msgBuf.size());
    std::ostringstream oss;
    oss << "POSIX error [errno=" << errno << "," << eMsg << "] " << msg;
    m_msg = oss.str();
}

PosixError::PosixError(const std::string& msg, int err)
: m_errno(err)
{
    std::array<char,128> msgBuf;
    const char*eMsg = strerror_r(m_errno,&msgBuf[0],msgBuf.size());
    std::ostringstream oss;
    oss << "POSIX error [errno=" << errno << "," << eMsg << "] " << msg;
    m_msg = oss.str();
}

void PosixError::ASSERT(bool truth, const std::string& msg)
{
    if (truth)
    {
        return;
    }
    throw PosixError(msg);
}

