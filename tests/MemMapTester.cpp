#include <gtest/gtest.h>
#include "MemMap.h"

using namespace posixcpp;

double mincore(void* ptr, size_t len)
{
    size_t numPages = (len+getpagesize()-1)/getpagesize();
    std::vector<unsigned char> vec(numPages);
    std::fill(vec.begin(),vec.end(),0);

    int r = ::mincore(ptr,len,&vec[0]);
    PosixError::ASSERT(r==0,"mincore");
    unsigned nRes = std::count_if(vec.begin(),vec.end(),
        [](unsigned char c)->bool{ return c&1;});

    return double(nRes)/double(numPages);
}

TEST(MemMap,basic)
{
    size_t sizeBytes = 0x100000;
    File file("/dev/zero",O_RDWR);
    auto map = memMap<char>(file,sizeBytes);

    // Fill memory
    memset(map.get(),'a',sizeBytes);
    char* ptr = map.get();
    double inCore = mincore(ptr,sizeBytes);
    ASSERT_EQ(1.0,inCore);

    // Unmap the pointer
    map.reset();

    // Verify it's no longer mapped
    ASSERT_THROW(inCore = mincore(ptr,sizeBytes),PosixError) << inCore;
    ASSERT_EQ(ENOMEM,errno);
}

TEST(MemMap,sizes)
{
    size_t sizeBytes = 0x10000;
    File file("/dev/zero",O_RDWR);
    MemMap<uint8_t> map8(file,sizeBytes);
    MemMap<uint32_t> map32(file,sizeBytes);
    ASSERT_EQ(sizeBytes,map8.sizeBytes());
    ASSERT_EQ(sizeBytes,map32.sizeBytes());
    ASSERT_EQ(sizeBytes,map8.len());
    ASSERT_EQ(sizeBytes/sizeof(uint32_t),map32.len());
}

TEST(MemMap,rainy)
{
    size_t sizeBytes = 0x10000;
    File file;
    ASSERT_NO_THROW(file = File("/dev/full",O_RDWR));
    EXPECT_THROW(MemMap<uint8_t> map8(file,sizeBytes),PosixError);
    ASSERT_EQ(ENODEV,errno);
}

