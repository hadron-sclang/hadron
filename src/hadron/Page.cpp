#include "hadron/Page.hpp"

#include "spdlog/spdlog.h"

#include <errno.h>
#include <string.h>

#if WIN32
#    define WIN32_LEAN_AND_MEAN
#    include "windows.h"
#else
#    include <sys/mman.h>
#endif // WIN32

namespace hadron {

Page::Page(int32_t objectSize, int32_t totalSize):
    m_startAddress(nullptr),
    m_objectSize(objectSize),
    m_totalSize(totalSize),
    m_nextFreeObject(0),
    m_allocatedObjects(0) {
    m_collectionCounts.resize(totalSize / objectSize, 0);
}

Page::~Page() { unmap(); }

bool Page::map() {
    if (m_startAddress) {
        SPDLOG_WARN("Duplicate calls to Page::map()");
        return true;
    }

#if WIN32
    void* address = VirtualAlloc(nullptr, m_totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    bool success = (address != nullptr);
#else
    void* address = mmap(nullptr, m_totalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    bool success = (address != MAP_FAILED);
#endif

    if (!success) {
        SPDLOG_CRITICAL("VM Page map failed for {} bytes", m_totalSize);
        return false;
    }

    m_startAddress = reinterpret_cast<int8_t*>(address);
    return true;
}

bool Page::unmap() {
    if (m_startAddress == nullptr) {
        return true;
    }

#if WIN32
    bool success = VirtualFree(m_startAddress, 0, MEM_RELEASE);
#else
    bool success = munmap(m_startAddress, m_totalSize) == 0;
#endif

    if (!success) {
        SPDLOG_ERROR("Page munmap failed");
        return false;
    }

    m_startAddress = nullptr;
    return true;
}

void* Page::allocate() {
    if (m_allocatedObjects == static_cast<int32_t>(m_collectionCounts.size())) {
        return nullptr;
    }
    assert(m_collectionCounts[m_nextFreeObject] == 0);
    void* address = m_startAddress + (m_nextFreeObject * m_objectSize);
    m_collectionCounts[m_nextFreeObject] = 1;
    ++m_allocatedObjects;
    if (m_allocatedObjects < static_cast<int32_t>(m_collectionCounts.size())) {
        for (size_t i = 1; i < m_collectionCounts.size(); ++i) {
            m_nextFreeObject = (m_nextFreeObject + i) % static_cast<int32_t>(m_collectionCounts.size());
            if (m_collectionCounts[m_nextFreeObject] == 0) {
                break;
            }
        }
    } else {
        m_nextFreeObject = static_cast<int32_t>(m_collectionCounts.size());
    }
    return address;
}

int32_t Page::capacity() {
    assert(m_allocatedObjects <= static_cast<int32_t>(m_collectionCounts.size()));
    return static_cast<int32_t>(m_collectionCounts.size()) - m_allocatedObjects;
}

void Page::mark(void* address, Color color) {
    intptr_t start = reinterpret_cast<intptr_t>(m_startAddress);
    intptr_t addressInt = reinterpret_cast<intptr_t>(address);
    assert(start <= addressInt);
    assert(addressInt - start < m_totalSize);
    size_t objectNumber = (addressInt - start) / m_objectSize;
    // Strip out old color if any.
    m_collectionCounts[objectNumber] &= 0x3f;
    m_collectionCounts[objectNumber] |= color;
}

} // namespace hadron