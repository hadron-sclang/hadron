#ifndef SRC_COMPILER_INCLUDE_HADRON_HEAP_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_HEAP_HPP_

#include <cstddef>
#include <vector>

namespace hadron {

// Manages dynamic memory allocation for Hadron, including garbage collection. Inspired by the design of the v8 garbage
// collection system, but greatly simplified.
class Heap {
public:
    Heap();
    ~Heap();
    // Map some initial pages, set up 
    bool setup();

    void* allocateNew(size_t bytes);

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
        kExtraLarge = 3
    };
    SizeClass getSizeClass(size_t bytes);
    

};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_HEAP_HPP_