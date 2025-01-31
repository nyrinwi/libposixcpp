#include "File.h"
#include "PosixError.h"
#include <gtest/gtest.h>

using namespace posixcpp;

TEST(PosixError,basic)
{
    std::ostringstream oss;

    open("lsajdff",O_RDONLY);
    PosixError e("wtf");
    ASSERT_EQ(ENOENT,e.errnoVal());
    std::string ss = e.what();
    auto ii = ss.find("wtf");
    ASSERT_NE(std::string::npos,ii) << ss;
    try {
        throw PosixError("",ENOANO);
    }
    catch(const std::exception & e)
    {
    }
}

TEST(PosixError,ASSERT)
{
    EXPECT_THROW(PosixError::ASSERT(false),PosixError);
}
