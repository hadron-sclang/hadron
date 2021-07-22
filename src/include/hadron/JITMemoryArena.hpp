#ifndef SRC_INCLUDE_HADRON_JIT_MEMORY_ARENA_HPP_
#define SRC_INCLUDE_HADRON_JIT_MEMORY_ARENA_HPP_

#include <cstddef>

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
    void* alloc(size_t bytes);
    void* realloc(void*, size_t) // todo

    void destroyArena();

private:
    unsigned m_arenaID;
    extent_hooks_t* m_hooks;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_JIT_MEMORY_ARENA_HPP_