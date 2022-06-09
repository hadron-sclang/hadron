#include "hadron/Page.hpp"

#include "spdlog/spdlog.h"

#include <errno.h>
#include <string.h>
#include <sys/mman.h>

namespace hadron {

Page::Page(size_t objectSize, size_t totalSize, bool isExecutable):
    m_startAddress(nullptr),
    m_objectSize(objectSize),
    m_totalSize(totalSize),
    m_isExecutable(isExecutable),
    m_nextFreeObject(0),
    m_allocatedObjects(0) {
    m_collectionCounts.resize(totalSize / objectSize, 0);
}

Page::~Page() {
    unmap();
}

bool Page::map() {
    if (m_startAddress) {
        SPDLOG_WARN("Duplicate calls to Page::map()");
        return true;
    }

    void* address = MAP_FAILED;
    if (!m_isExecutable) {
        address = mmap(nullptr, m_totalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    } else {
#if defined(__APPLE__)
        address = mmap(nullptr, m_totalSize, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_JIT | MAP_PRIVATE | MAP_ANONYMOUS,
                -1, 0);
#else
        address = mmap(nullptr, m_totalSize, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    }

    if (address == MAP_FAILED) {
        int mmapError = errno;
        SPDLOG_CRITICAL("Page mmap failed for {} bytes, errno: {}, string: {}", m_totalSize, mmapError,
                strerror(mmapError));
        return false;
    }

    m_startAddress = reinterpret_cast<uint8_t*>(address);
    return true;
}

bool Page::unmap() {
    if (m_startAddress == nullptr) {
        return true;
    }

    if (munmap(m_startAddress, m_totalSize) != 0) {
        int munmapError = errno;
        SPDLOG_ERROR("Page munmap failed for with errno: {}, string: {}", munmapError, strerror(munmapError));
        return false;
    }

    m_startAddress = nullptr;
    return true;
}

void* Page::allocate() {
    if (m_allocatedObjects == m_collectionCounts.size()) {
        return nullptr;
    }
    assert(m_collectionCounts[m_nextFreeObject] == 0);
    void* address = m_startAddress + (m_nextFreeObject * m_objectSize);
    m_collectionCounts[m_nextFreeObject] = 1;
    ++m_allocatedObjects;
    if (m_allocatedObjects < m_collectionCounts.size()) {
        for (size_t i = 1; i < m_collectionCounts.size(); ++i) {
            m_nextFreeObject = (m_nextFreeObject + i) % m_collectionCounts.size();
            if (m_collectionCounts[m_nextFreeObject] == 0) {
                break;
            }
        }
    } else {
        m_nextFreeObject = m_collectionCounts.size();
    }
    return address;
}

size_t Page::capacity() {
    assert(m_allocatedObjects <= m_collectionCounts.size());
    return m_collectionCounts.size() - m_allocatedObjects;
}

void Page::mark(void* address, Color color) {
    uintptr_t start = reinterpret_cast<uintptr_t>(m_startAddress);
    uintptr_t addressInt = reinterpret_cast<uintptr_t>(address);
    assert(start <= addressInt);
    assert(addressInt - start < m_totalSize);
    size_t objectNumber = (addressInt - start) / m_objectSize;
    // Strip out old color if any.
    m_collectionCounts[objectNumber] &= 0x3f;
    m_collectionCounts[objectNumber] |= color;
}

} // namespace hadron