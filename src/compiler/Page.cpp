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
    m_highWaterMark(0) {}

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
        address = mmap(nullptr, m_totalSize, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_JIT | MAP_PRIVATE | MAP_ANONYMOUS,
                -1, 0);
    }

    if (address == MAP_FAILED) {
        int mmapError = errno;
        SPDLOG_CRITICAL("Page mmap failed for {} bytes, errno: {}, string: {}", sizeInBytes, mmapError,
                strerror(mmapError));
        return false;
    }

    m_startAddress = address;
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


} // namespace hadron