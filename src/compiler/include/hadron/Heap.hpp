#ifndef SRC_COMPILER_INCLUDE_HADRON_HEAP_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_HEAP_HPP_

#include <array>
#include <cstddef>
#include <vector>

namespace hadron {
class Page;

// Manages dynamic memory allocation for Hadron, including garbage collection. Inspired by the design of the v8 garbage
// collection system, but greatly simplified.
class Heap {
public:
    Heap();
    ~Heap();

    // Map some initial pages, set up for low(er)-latency allocations.
    bool setup();

    // Default allocation, allocates from the young space (unless extra large).
    void* allocateNew(size_t sizeInBytes);

    // TODO: verify size classes experimentally.
    static constexpr size_t kSmallObjectSize = 256;
    static constexpr size_t kMediumObjectSize = 2048;
    static constexpr size_t kLargeObjectSize = 16384;
    static constexpr size_t kPageSize = 256 * 1024;

private:
    enum SizeClass {
        kSmall = 0,
        kMedium = 1,
        kLarge = 2,
        kExtraLarge = 3,
        kNumberOfSizeClasses = 4
    };
    SizeClass getSizeClass(size_t sizeInBytes);
    std::array<std::vector<Page>, kNumberOfSizeClasses> m_youngPages;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_HEAP_HPP_