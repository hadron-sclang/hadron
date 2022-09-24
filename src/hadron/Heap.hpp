#ifndef SRC_COMPILER_INCLUDE_HADRON_HEAP_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_HEAP_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Page.hpp"
#include "hadron/Slot.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hadron {

namespace library {
struct Schema;
}

// Manages dynamic memory allocation for Hadron, including garbage collection. Inspired by the design of the v8 garbage
// collection system, but greatly simplified.
class Heap {
public:
    Heap() = default;
    ~Heap() = default;

    // Default allocation, allocates from the young space (unless extra large). Does not initialize the memory to
    // a known value.
    void* allocateNew(int32_t sizeInBytes, int32_t& allocatedSize);

    // Adds to the list of permanent objects that are the point of origin for all scanning jobs.
    void addToRootSet(Slot object);
    void removeFromRootSet(Slot object);

    // TODO: verify size classes experimentally.
    static constexpr int32_t kSmallObjectSize = 256;
    static constexpr int32_t kMediumObjectSize = 2048;
    static constexpr int32_t kLargeObjectSize = 32 * 1024;
    static constexpr int32_t kPageSize = 256 * 1024;

    // Given a pointer, return the object that this pointer refers to (may be an inner address of that object), or
    // nullptr if this pointer doesn't point at a valid, active object.
    library::Schema* getContainingObject(const void* address);

private:
    enum SizeClass { kSmall = 0, kMedium = 1, kLarge = 2, kOversize = 3, kNumClasses = 4 };
    using SizedPages = std::array<std::vector<std::unique_ptr<Page>>, kNumClasses>;

    SizeClass getSizeClass(int32_t sizeInBytes);
    int32_t getSize(SizeClass sizeClass);
    void* allocateSized(int32_t sizeInBytes, int32_t& allocatedSize, SizedPages& sizedPages);

    // Return a pointer to a Page object that contains the provided address.
    Page* findPageContaining(const void* address);

    SizedPages m_youngPages;
    SizedPages m_maturePages;

    // Not garbage collected, permanently allocated objects. Root objects are where scanning starts, along with the
    // stack.
    std::unordered_set<library::Schema*> m_rootSet;

    // Address of first byte past the end of each Page, used for mapping an arbitrary address back to owning object.
    std::map<intptr_t, Page*> m_pageEnds;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_HEAP_HPP_