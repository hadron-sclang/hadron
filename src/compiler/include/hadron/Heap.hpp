#ifndef SRC_COMPILER_INCLUDE_HADRON_HEAP_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_HEAP_HPP_

#include "hadron/Page.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <vector>

namespace hadron {

// Manages dynamic memory allocation for Hadron, including garbage collection. Inspired by the design of the v8 garbage
// collection system, but greatly simplified.
class Heap {
public:
    Heap();
    ~Heap();

    // Default allocation, allocates from the young space (unless extra large).
    void* allocateNew(size_t sizeInBytes);

    // Used for allocating JIT memory. Returns the maximum usable size in |allocatedSize|, which can be useful as the
    // JIT bytecode is typically based on size estimates.
    void* allocateJIT(size_t sizeInBytes, size_t& allocatedSize);

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
        kNumberOfSizeClasses = 3
    };
    SizeClass getSizeClass(size_t sizeInBytes);
    size_t getSize(SizeClass sizeClass);
    void mark();
    void sweep();

    // Nonfull pages organized by size class, already mapped to (hopefully) minimize typical allocation latency.
    std::array<std::vector<Page>, kNumberOfSizeClasses> m_youngPages;
    // Young objects that survive a few collections are compacted and copied to m_oldPages.
    std::array<std::vector<Page>, kNumberOfSizeClasses> m_maturePages;

    std::array<std::vector<Page>, kNumberOfSizeClasses> m_youngExecutablePages;
    std::array<std::vector<Page>, kNumberOfSizeClasses> m_matureExecutablePages;

    size_t m_totalMappedPages;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_HEAP_HPP_