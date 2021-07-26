#ifndef SRC_INCLUDE_HADRON_JIT_MEMORY_ARENA_HPP_
#define SRC_INCLUDE_HADRON_JIT_MEMORY_ARENA_HPP_

#include <cstddef>
#include <functional>
#include <memory>

extern "C" {
typedef struct extent_hooks_s extent_hooks_t;
}

namespace hadron {

// All the supported operating systems require some sort of special allocation strategy, and some require additional
// security considerations, in order to mark memory as both writeable and executable. It is also recommended to
// allocate a few comparatively large blocks of memory and JIT many pieces of code into them, to keep system resources
// down, as opposed to a single allocation per JITted block of code. Lastly, because typical Hadron usage includes the
// execution of a large amount of ephemeral code, we use a custom memory allocator jemalloc to track JIT memory in a
// dedicated memory arena, separate from the main arena and the garbage-collected arena.
class JITMemoryArena {
public:
    JITMemoryArena();
    ~JITMemoryArena();  // also calls destroyArena()

    bool createArena();
    // MCodePtr uses a custom deleter to call free() on this arena when deleting the memory allocated by alloc().
    using MCodePtr = std::unique_ptr<uint8_t, std::function<void(uint8_t*)>>;
    MCodePtr alloc(size_t size);
    void resize(void*, size_t); // todo

    void destroyArena();

private:
    void freeMCode(uint8_t* mcode);

    unsigned m_arenaID;
    extent_hooks_t* m_hooks;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_JIT_MEMORY_ARENA_HPP_