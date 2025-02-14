#include <array>
#include "SocketPair.h"
#include "PosixError.h"
#include <gtest/gtest.h>

using namespace posixcpp;

TEST(SocketPair,stream)
{
    std::array<char,12> message{"Hello World"};
    std::array<char,12> response;

    // socketpair
    SocketPair socket(AF_UNIX,SOCK_STREAM);
    ASSERT_TRUE(socket.reader().fdValid());
    ASSERT_TRUE(socket.writer().fdValid());

    socket.writer().write(&message[0],message.size());
    socket.reader().read(&response[0],message.size());
    ASSERT_EQ(message,response);

    socket.writer().write(&message[0],message.size());
    socket.reader().read(&response[0],message.size());
    ASSERT_EQ(message,response);
}

TEST(SocketPair,dgram)
{
    std::array<char,12> message{"Hello World"};
    std::array<char,12> response;

    // socketpair
    SocketPair socket(AF_UNIX,SOCK_DGRAM);
    ASSERT_TRUE(socket.reader().fdValid());
    ASSERT_TRUE(socket.writer().fdValid());

    socket.writer().write(&message[0],message.size());
    socket.reader().read(&response[0],message.size());
    ASSERT_EQ(message,response);

    socket.writer().write(&message[0],message.size());
    socket.reader().read(&response[0],message.size());
    ASSERT_EQ(message,response);
}
