#include <array>
#include "Pipe.h"
#include "PosixError.h"
#include <gtest/gtest.h>

using namespace posixcpp;

TEST(Pipe,basic)
{
    auto pipe = Pipe();
    std::array<char,12> message{"Hello World"};
    std::array<char,12> response;

    pipe.writer().write(&message[0],message.size());
    pipe.reader().read(&response[0],message.size());
    ASSERT_EQ(message,response);
}

