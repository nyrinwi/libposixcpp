#include "Pipe.h"
#include "PosixError.h"
#include <gtest/gtest.h>

using namespace posixcpp;

TEST(Pipe,basic)
{
    auto pipe = Pipe();
}
