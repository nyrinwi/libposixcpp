#include <gtest/gtest.h>
#include "MemMap.h"

using namespace posixcpp;

TEST(MemMap,basic)
{
    File file("/dev/zero",O_RDWR);
    auto map = memMap<char>(file,0x100000);
    snprintf(map.get(),0x1000,"Hello World\n");
    std::cout << map.get();
    map.reset();
}

TEST(MemMap,obj)
{
    size_t size = 0x10000;
    File file("/dev/zero",O_RDWR);
    MemMap<char> cmap(file,size);
    MemMap<uint32_t> wmap(file,size);
    ASSERT_EQ(size,cmap.sizeBytes());
    ASSERT_EQ(size,wmap.sizeBytes());
    ASSERT_EQ(size,cmap.len());
    ASSERT_EQ(size/sizeof(uint32_t),wmap.len());
}

