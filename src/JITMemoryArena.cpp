#include "hadron/JITMemoryArena.hpp"

#include "fmt/format.h"
#include "jemalloc/jemalloc.h"

#include <pthread.h>  // for apple jit magic
#include <string>
#include <sys/mman.h>

namespace {

void* jitArenaAlloc(extent_hooks_t* /* extent */, void* addr, size_t size, size_t /* alignment */, bool* zero,
        bool* commit, unsigned /* arenaIndex */) {
    *zero = false;
    *commit = false;
    void* alloc = mmap(addr, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_JIT | MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0);
    return alloc;
}

bool jitArenaDealloc(extent_hooks_t* /* extent */, void * addr, size_t size, bool /* committed */,
        unsigned /* arenaIndex */) {
    return munmap(addr, size) == 0;
}

}

namespace hadron {

JITMemoryArena::JITMemoryArena(): m_arenaID(0), m_hooks(nullptr) {}
JITMemoryArena::~JITMemoryArena() { destroyArena(); }

bool JITMemoryArena::createArena() {
    m_hooks = reinterpret_cast<extent_hooks_t*>(malloc(sizeof(extent_hooks_t)));
    m_hooks->alloc = jitArenaAlloc;
    m_hooks->dalloc = jitArenaDealloc;
    m_hooks->destroy = nullptr;
    m_hooks->commit = nullptr;
    m_hooks->decommit = nullptr;
    m_hooks->purge_lazy = nullptr;
    m_hooks->purge_forced = nullptr;
    m_hooks->split = nullptr;
    m_hooks->merge = nullptr;
    size_t idSize = sizeof(m_arenaID);
    int result = je_mallctl("arenas.create", reinterpret_cast<void*>(&m_arenaID), &idSize,
        reinterpret_cast<void*>(&m_hooks), sizeof(extent_hooks_t*));
    if (result != 0) {
        free(m_hooks);
        m_hooks = nullptr;
        return false;
    }
    return true;
}

JITMemoryArena::MCodePtr JITMemoryArena::alloc(size_t size) {
    constexpr size_t kJITMemAlign = 16;
    constexpr size_t kJITMemAlignMask = ~(0xf);
    size = (size + kJITMemAlign - 1) & kJITMemAlignMask;

    // Preserve current thread arena. TODO: mallctlnametomib for thread.arena
    unsigned arena;
    size_t arenaSize = sizeof(arena);
    int result = je_mallctl("thread.arena", reinterpret_cast<void*>(&arena), &arenaSize, nullptr, 0);
    if (result != 0) {
        return nullptr;
    }

    // Switch thread arena to the jit one.
    result = je_mallctl("thread.arena", nullptr, nullptr, reinterpret_cast<void*>(&m_arenaID), sizeof(unsigned));
    if (result != 0) {
        return nullptr;
    }

    // Allocate the memory.
    void* memory = je_aligned_alloc(kJITMemAlign, size);
    auto mcode = MCodePtr(memory, [this](void* ptr) { free(ptr); });

    // Restore old thread arena.
    result = je_mallctl("thread.arena", nullptr, nullptr, reinterpret_cast<void*>(&arena), sizeof(unsigned));
    if (result != 0) {
        return nullptr;
    }

    return mcode;
}

void JITMemoryArena::destroyArena() {
    if (m_hooks) {
        std::string arenaDestroy = fmt::format("arena.{}.destroy", m_arenaID);
        je_mallctl(arenaDestroy.data(), nullptr, nullptr, nullptr, 0);
        free(m_hooks);
        m_hooks = nullptr;
    }
}

} // namespace hadron