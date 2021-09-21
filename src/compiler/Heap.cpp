#include "hadron/Heap.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

Heap::Heap(): m_totalMappedPages(0) {}

void* Heap::allocateNew(size_t sizeInBytes) {
    auto sizeClass = getSizeClass(sizeInBytes);
    if (sizeClass == kNumberOfSizeClasses) {
        SPDLOG_ERROR("Heap failed allocating very large object of size {}", sizeInBytes);
        return nullptr;
    }

    for (auto& page : m_youngPages[sizeClass]) {
        if (page.capacity()) {
            return page.allocate();
        }
    }

    void* address = nullptr;
    if (m_youngPages[sizeClass].size() < 2) {
        m_youngPages[sizeClass].emplace_back(Page{getSize(sizeClass), kPageSize});
        if (!m_youngPages[sizeClass].back().map()) {
            return nullptr;
        }
        address = m_youngPages[sizeClass].back().allocate();
    }

    mark();
    sweep();

    return address;
}

Heap::SizeClass Heap::getSizeClass(size_t sizeInBytes) {
    if (sizeInBytes < kSmallObjectSize) {
        return SizeClass::kSmall;
    } else if (sizeInBytes < kMediumObjectSize) {
        return SizeClass::kMedium;
    } else if (sizeInBytes < kLargeObjectSize) {
        return SizeClass::kLarge;
    }
    return SizeClass::kNumberOfSizeClasses;
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

void Heap::mark() {

}

void Heap::sweep() {

}

} // namespace hadron