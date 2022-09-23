#include "hadron/Heap.hpp"

#include "hadron/Slot.hpp"
#include "spdlog/spdlog.h"

#include <cassert>
#include <sys/_types/_uintptr_t.h>

namespace hadron {

void* Heap::allocateNew(int32_t sizeInBytes, int32_t& allocatedSize) {
    return allocateSized(sizeInBytes, allocatedSize, m_youngPages);
}

void Heap::addToRootSet(Slot object) { m_rootSet.emplace(object.getPointer()); }

void Heap::removeFromRootSet(Slot object) { m_rootSet.erase(object.getPointer()); }

library::Schema* Heap::getContainingObject(const void* address) {
    Page* page = findPageContaining(address);
    if (!page) {
        return nullptr;
    }
    // Align pointer on allocation size
    uintptr_t aligned =
        reinterpret_cast<uintptr_t>(address) - (reinterpret_cast<uintptr_t>(address) % page->objectSize());

    // TODO: aliveness verification? How?
    return reinterpret_cast<library::Schema*>(aligned);
}

Heap::SizeClass Heap::getSizeClass(int32_t sizeInBytes) {
    if (sizeInBytes <= kSmallObjectSize) {
        return SizeClass::kSmall;
    } else if (sizeInBytes <= kMediumObjectSize) {
        return SizeClass::kMedium;
    } else if (sizeInBytes <= kLargeObjectSize) {
        return SizeClass::kLarge;
    }
    return SizeClass::kOversize;
}

int32_t Heap::getSize(SizeClass sizeClass) {
    switch (sizeClass) {
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

void* Heap::allocateSized(int32_t sizeInBytes, int32_t& allocatedSize, SizedPages& sizedPages) {
    auto sizeClass = getSizeClass(sizeInBytes);
    if (sizeClass == kOversize) {
        assert(false);
        sizedPages[kOversize].emplace_back(std::make_unique<Page>(sizeInBytes, sizeInBytes));
        if (!sizedPages[kOversize].back()->map()) {
            SPDLOG_ERROR("Mapping failed for oversize object of {} bytes", sizeInBytes);
            return nullptr;
        }
        allocatedSize = sizeInBytes;
        // We leave oversize pages out of page address map, and search the oversized pages separately.
        return sizedPages[kOversize].back()->allocate();
    }

    allocatedSize = getSize(sizeClass);

    // Find existing capacity in already mapped pages.
    for (auto& page : sizedPages[sizeClass]) {
        if (page->capacity()) {
            return page->allocate();
        }
    }

    // HERE is where we would initiate a collection

    sizedPages[sizeClass].emplace_back(std::make_unique<Page>(getSize(sizeClass), kPageSize));
    if (!sizedPages[sizeClass].back()->map()) {
        assert(false);
        return nullptr;
    }

    auto address = sizedPages[sizeClass].back()->startAddress();
    assert(address);
    m_pageEnds[reinterpret_cast<intptr_t>(address) + kPageSize] = sizedPages[sizeClass].back().get();

    return sizedPages[sizeClass].back()->allocate();
}

Page* Heap::findPageContaining(const void* address) {
    auto addressValue = reinterpret_cast<intptr_t>(address);
    auto page = m_pageEnds.upper_bound(addressValue);
    if (page == m_pageEnds.end()) {
        return nullptr;
    }
    auto startAddress = reinterpret_cast<intptr_t>(page->second->startAddress());
    assert(addressValue >= startAddress);
    if (addressValue - startAddress < static_cast<int32_t>(page->second->totalSize())) {
        return page->second;
    }
    // TODO: search oversize
    return nullptr;
}

} // namespace hadron