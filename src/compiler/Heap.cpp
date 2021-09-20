#include "hadron/Heap.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

Heap::Heap():
    m_youngPages{
        Page{kSmallObjectSize, kPageSize},
        Page{kMediumObjectSize, kPageSize},
        Page{kLargeObjectSize, kPageSize}},
    m_totalMappedPages(0) {}

bool Heap::setup() {
    if (!m_youngPages[kSmall].map()) {
        return false;
    }
    ++m_totalMappedPages;

    if (!m_youngPages[kMedium].map()) {
        return false;
    }
    ++m_totalMappedPages;

    if (!m_youngPages[kLarge].map()) {
        return false;
    }
    ++m_totalMappedPages;

    return true;
}

void* Heap::allocateNew(size_t sizeInBytes) {
    auto sizeClass = getSizeClass(sizeInBytes);
    if (sizeClass == kNumberOfSizeClasses) {
        SPDLOG_ERROR("Heap failed allocating very large object of size {}", sizeInBytes);
        return nullptr;
    }

    if (m_youngPages[sizeClass].capacity()) {
        return m_youngPages[sizeClass].allocate();
    }

    return nullptr;
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

} // namespace hadron