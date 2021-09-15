#include "hadron/Heap.hpp"

namespace hadron {

// Pages should be aligned along page size.
struct Heap::Page {
    void* pageStartAddress;
    size_t highWaterMark;
    int mmapFileDescriptor;
};

struct Heap::Space {
    std::vector<Heap::Page> freePages;
};

void* Heap::allocateNew(size_t bytes) {

}

Heap::SizeClass Heap::getSizeClass(size_t bytes) {
    if (bytes < kSmallObjectSize) {
        return SizeClass::kSmall;
    } else if (bytes < kMediumObjectSize) {
        return SizeClass::kMedium;
    } else if (bytes < kLargeObjectSize) {
        return SizeClass::kLarge;
    }
    return SizeClass::kExtraLarge;
}

} // namespace hadron