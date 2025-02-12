#ifndef MEMMAP_H
#define MEMMAP_H

#include <sys/mman.h>
#include <memory>
#include "File.h"
#include "PosixError.h"

namespace posixcpp
{

enum MmapFlags {
    MAP_SHARED_=MAP_SHARED,
    MAP_PRIVATE_=MAP_PRIVATE,
    MAP_ANONYMOUS_=MAP_ANONYMOUS,
};

enum MmapProt {
    PROT_READ_=PROT_READ,
    PROT_WRITE_=PROT_WRITE,
    PROT_RW_=PROT_READ|PROT_WRITE,
};

// Generic mmap function returns a naked pointer
template <typename Typ>
Typ* memMapCore(posixcpp::File& file, size_t size, size_t offset=0,MmapFlags flags=MAP_SHARED_, MmapProt prot=PROT_RW_)
{
    // Note we do not keep a reference to file because we don't need to
    void* ptr = mmap(0,size,prot,flags,file.fd(),offset);
    PosixError::ASSERT(ptr!=MAP_FAILED);
    return reinterpret_cast<Typ*>(ptr);
}

// Generic mmap function returns a smart pointer
template <typename Typ>
std::shared_ptr<Typ> memMap(posixcpp::File& file, size_t size, size_t offset=0,MmapFlags flags=MAP_SHARED_, MmapProt prot=PROT_RW_)
{
    using namespace posixcpp;
    auto ptr = memMapCore<Typ>(file,size,offset,flags,prot);
    auto deleter = [size](void* p) {munmap(p,size);};
    return std::shared_ptr<Typ>(reinterpret_cast<Typ*>(ptr),deleter);
}

/*
** Note that regardless of the type, the size is always in bytes
*/
template <typename Typ>
class MemMap
{
protected:
    size_t m_sizeBytes;
    std::shared_ptr<Typ> m_ptr;
public:
    MemMap(File& file, size_t size, size_t offset=0,MmapFlags flags=MAP_SHARED_, MmapProt prot=PROT_RW_)
    : m_sizeBytes(size),
      m_ptr(memMap<Typ>(file,size,offset,flags,prot))
    {
    };

    size_t sizeBytes() const
    {
        return m_sizeBytes;
    };

    size_t len() const
    {
        return m_sizeBytes/sizeof(Typ);
    };

    size_t numPages() const
    {
        return (sizeBytes()+getpagesize()-1)/getpagesize();
    }

    Typ* get() const
    {
        return m_ptr.get();
    };
};

}

#endif

