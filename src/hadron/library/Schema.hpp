#ifndef SRC_HADRON_LIBRARY_SCHEMA_HPP_
#define SRC_HADRON_LIBRARY_SCHEMA_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"

#include <type_traits>

namespace hadron {
namespace library {

// Object instances in Hadron are contiguous blocks of Slots. The root SC Object has no instance variables accessible
// from the language, but descendent objects that have instance variables are appended on in declaration order. Objects
// with primitives are precompiled as structs that give their names to member variables and wrap some C++ around the
// runtime objects.
struct Schema {
    // This and all derived objects should never be made on the heap, rather should be constructed in the
    // garbage-collected space.
    Schema() = delete;
    Schema(const Schema&) = delete;
    Schema(const Schema&&) = delete;
    ~Schema() = delete;

    // Underscores as prefixes for these members so they don't collide with instance variables derived from scanning the
    // SuperCollider class files. This is always a symbol.
    Hash _className;
    // This is absolute size, including this header.
    uint64_t _sizeInBytes;
};

// Important we not have a vtable in these objects, so no virtual functions.
static_assert(std::is_standard_layout<Schema>::value);

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_SCHEMA_HPP_