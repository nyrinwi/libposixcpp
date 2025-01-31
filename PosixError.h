#include <stdexcept>
#include <string>

namespace posixcpp
{

class PosixError : public std::exception
{
protected:
    std::string m_msg;
    const int m_errno;

public:
    PosixError(const std::string& msg);

    PosixError(const std::string& msg, int err);

    int errnoVal() const noexcept
    {
        return m_errno;
    }

    const char* what() const noexcept override
    {
        return m_msg.c_str();
    };

    static void ASSERT(bool truth, const std::string& msg="");
};

};
