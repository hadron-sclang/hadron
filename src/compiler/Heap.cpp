#include "hadron/Heap.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

Heap::Heap(): m_stackPageOffset(0), m_totalMappedPages(0) {}

void* Heap::allocateNew(size_t sizeInBytes) {
    return allocateSized(sizeInBytes, m_youngPages, false);
}

void* Heap::allocateJIT(size_t sizeInBytes, size_t& allocatedSize) {
    auto address = allocateSized(sizeInBytes, m_executablePages, true);
    if (address) {
        auto sizeClass = getSizeClass(sizeInBytes);
        allocatedSize = getSize(sizeClass);
    } else {
        allocatedSize = 0;
    }
    return address;
}

void* Heap::allocateStackSegment() {
    if (m_stackPageOffset == 0) {
        m_stackSegments.emplace_back(Page{kLargeObjectSize, kPageSize, false});
        if (!m_stackSegments.back().map()) {
            SPDLOG_CRITICAL("Failed to map new stack segment.");
            assert(false);
            return nullptr;
        }
    }

    auto address = m_stackSegments.back().startAddress();
    assert(address);
    address += m_stackPageOffset;

    m_stackPageOffset = (m_stackPageOffset + kLargeObjectSize) % kPageSize;
    return address;
}

void Heap::freeTopStackSegment() {
    // TODO: consider some page recycling or other hysterisis here to prevent stack boundary oscillation from spamming
    // map/unmap syscalls.
    if (m_stackPageOffset == 0) {
        assert(m_stackSegments.size());
        m_stackSegments.pop_back();
        m_stackPageOffset = kPageSize - kLargeObjectSize;
    } else {
        assert(m_stackPageOffset >= kLargeObjectSize);
        m_stackPageOffset -= kLargeObjectSize;
    }
}

void* Heap::allocateRootSet(size_t sizeInBytes) {
    return allocateSized(sizeInBytes, m_rootSet, false);
}

Heap::SizeClass Heap::getSizeClass(size_t sizeInBytes) {
    if (sizeInBytes < kSmallObjectSize) {
        return SizeClass::kSmall;
    } else if (sizeInBytes < kMediumObjectSize) {
        return SizeClass::kMedium;
    } else if (sizeInBytes < kLargeObjectSize) {
        return SizeClass::kLarge;
    }
    return SizeClass::kOversize;
}

size_t Heap::getSize(SizeClass sizeClass) {
    switch(sizeClass) {
    case kSmall:
        return kSmallObjectSize;
    case kMedium:
        return kMediumObjectSize;
    case kLarge:
        return kLargeObjectSize;
    default:
        return 0;
    }
}

void* Heap::allocateSized(size_t sizeInBytes, SizedPages& sizedPages, bool isExecutable) {
    auto sizeClass = getSizeClass(sizeInBytes);
    if (sizeClass == kOversize) {
        sizedPages[kOversize].emplace_back(Page{sizeInBytes, sizeInBytes, isExecutable});
        if (!sizedPages[kOversize].back().map()) {
            SPDLOG_ERROR("Mapping failed for oversize object of {} bytes", sizeInBytes);
            return nullptr;
        }
        return sizedPages[kOversize].back().allocate();
    }

    // Find existing capacity in already mapped pages.
    for (auto& page : sizedPages[sizeClass]) {
        if (page.capacity()) {
            return page.allocate();
        }
    }

    // HERE is where we would initiate a collection
    mark();
    sweep();

    sizedPages[sizeClass].emplace_back(Page{getSize(sizeClass), kPageSize, isExecutable});
    if (!sizedPages[sizeClass].back().map()) {
        return nullptr;
    }
    ++m_totalMappedPages;
    return sizedPages[sizeClass].back().allocate();
}

void Heap::mark() {
    // TODO once we add the Stack should be enough to build a decent root set
}

void Heap::sweep() {
    // TODO once we have mark() going
}

} // namespace hadron