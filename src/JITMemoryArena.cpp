#include "hadron/JITMemoryArena.hpp"

#include <jemalloc/jemalloc.h>

namespace {

}

namespace hadron {

JITMemoryArena::JITMemoryArena(): m_arenaID(0) {}
JITMemoryArena::~JITMemoryArena() {}

bool JITMemoryArena::createArena() {
    // Cheap hack to create the default arena, since we are currently not using jemalloc for regular memory allocation.
//    void* oneByte = je_malloc(1);
//    if (!oneByte) {
//        return false;
//    }

    // Read the default function hook pointers from the jemalloc default arena.
    extent_hooks_t* defaultHooks;
    size_t defaultHooksSize = sizeof(defaultHooks);
    int result = je_mallctl("arena.0.extent_hooks", static_cast<void*>(&defaultHooks), &defaultHooksSize, nullptr, 0);
    if (result) {
        return false;
    }

//    je_free(oneByte);

    m_hooks = reinterpret_cast<extent_hooks_t*>(malloc(sizeof(extent_hooks_t)));
    if (!m_hooks) {
        return false;
    }
    m_hooks->alloc = defaultHooks->alloc;
    m_hooks->dalloc = defaultHooks->dalloc;
    m_hooks->destroy = defaultHooks->destroy;
    m_hooks->commit = defaultHooks->commit;
    m_hooks->decommit = defaultHooks->decommit;
    m_hooks->purge_lazy = defaultHooks->purge_lazy;
    m_hooks->purge_forced = defaultHooks->purge_forced;
    m_hooks->split = defaultHooks->split;
    m_hooks->merge = defaultHooks->merge;
    size_t idSize = sizeof(m_arenaID);
    result =  je_mallctl("arenas.create", static_cast<void*>(&m_arenaID), &idSize, static_cast<void*>(m_hooks),
        sizeof(extent_hooks_t*));
    return result != 0;
}

} // namespace hadron