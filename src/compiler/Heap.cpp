#include "hadron/Heap.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

Heap::Heap(): m_stackPageOffset(0), m_totalMappedPages(0) {}

void* Heap::allocateNew(size_t sizeInBytes) {
    return allocateYoung(sizeInBytes, m_youngPages, false);
}

void* Heap::allocateJIT(size_t sizeInBytes, size_t& allocatedSize) {
    auto address = allocateYoung(sizeInBytes, m_youngExecutablePages, true);
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

void* Heap::allocateYoung(size_t sizeInBytes, SizedPages& youngPages, bool isExecutable) {
    auto sizeClass = getSizeClass(sizeInBytes);
    if (sizeClass == kOversize) {
        youngPages[kOversize].emplace_back(Page{sizeInBytes, sizeInBytes, isExecutable});
        if (!youngPages[kOversize].back().map()) {
            SPDLOG_ERROR("Mapping failed for oversize object of {} bytes", sizeInBytes);
            return nullptr;
        }
        return youngPages[kOversize].back().allocate();
    }

    // Find existing capacity in already mapped pages.
    for (auto& page : youngPages[sizeClass]) {
        if (page.capacity()) {
            return page.allocate();
        }
    }

    // HERE is where we would initiate a collection
    mark();
    sweep();

    youngPages[sizeClass].emplace_back(Page{getSize(sizeClass), kPageSize, isExecutable});
    if (!youngPages[sizeClass].back().map()) {
        return nullptr;
    }
    ++m_totalMappedPages;
    return youngPages[sizeClass].back().allocate();
}

void Heap::mark() {
    // TODO once we add the Stack should be enough to build a decent root set
}

void Heap::sweep() {
    // TODO once we have mark() going
}

} // namespace hadron