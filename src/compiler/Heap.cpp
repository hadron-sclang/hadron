#include "hadron/Heap.hpp"

#include "hadron/Page.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

void* Heap::allocateNew(size_t sizeInBytes) {
    auto sizeClass = getSizeClass(sizeInBytes);
    if (sizeClass < SizeClass::kExtraLarge) {
        
    } else {

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
    return SizeClass::kExtraLarge;
}

} // namespace hadron