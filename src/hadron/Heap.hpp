#ifndef SRC_COMPILER_INCLUDE_HADRON_HEAP_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_HEAP_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Page.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <string_view>
#include <unordered_map>
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

    // Stack segments are always allocated at kLargeObjectSize. They are also allocated and freed in stack ordering,
    // and are exempt from garbage collection. They serve as part of the root set of objects for scanning.
    void* allocateStackSegment();
    void freeTopStackSegment();

    // Allocates to the set of permanent objects that are the point of origin for all scanning jobs, along with the
    // stack segments.
    void* allocateRootSet(size_t sizeInBytes);

    // Compute symbol hash, copy symbol data into root set symbol table (if not already set up), returns the Hash.
    Hash addSymbol(std::string_view symbol);

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
        kOversize = 3
    };
    using SizedPages = std::array<std::vector<Page>, kOversize>;

    SizeClass getSizeClass(size_t sizeInBytes);
    size_t getSize(SizeClass sizeClass);
    void* allocateSized(size_t sizeInBytes, SizedPages& sizedPages, bool isExecutable);
    void mark();
    void sweep();

    SizedPages m_youngPages;
    SizedPages m_maturePages;

    // Bytecode cannot be relocated and so is kept only in one set of pages.
    SizedPages m_executablePages;

    // Not garbage collected, permanently allocated objects. Root objects are where scanning starts, along with the
    // stack.
    SizedPages m_rootSet;

    std::vector<Page> m_stackSegments;
    // Offset of first free chunk within last Page of m_stackSegments.
    size_t m_stackPageOffset;

    size_t m_totalMappedPages;

    std::unordered_map<Hash, std::string_view> m_symbolTable;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_HEAP_HPP_