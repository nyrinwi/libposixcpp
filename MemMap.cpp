#include "File.h"
#include "MemMap.h"
#include <sys/mman.h>
#include <cassert>
#include <memory>

using namespace posixcpp;

std::shared_ptr<void>
map(File& file, size_t size, size_t offset=0,MmapFlags flags=MAP_SHARED_, MmapProt prot=PROT_RW_)
{
    void* ptr = mmap(0,size,prot,flags,file.fd(),offset);
    assert(ptr!=MAP_FAILED);
    auto deleter =  [size](void* p) {munmap(p,size);};
    return std::shared_ptr<void>(ptr,deleter);
}

