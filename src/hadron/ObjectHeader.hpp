#ifndef SRC_HADRON_OBJECT_HEADER_HPP_
#define SRC_HADRON_OBJECT_HEADER_HPP_

#include "hadron/Slot.hpp"

namespace hadron {

// Object instances in Hadron are contiguous blocks of Slots. The root SC Object has no instance variables accessible
// from the language, but descendent objects that have instance variables are appended on in declaration order. Objects
// with primitives are precompiled as structs that give their names to member variables and wrap some C++ around the
// runtime objects.
struct ObjectHeader {
    // This and all derived objects should never be made on the heap, rather should be constructed in the
    // garbage-collected space.
    ObjectHeader() = delete;
    ObjectHeader(const ObjectHeader&) = delete;
    ObjectHeader(const ObjectHeader&&) = delete;
    ~ObjectHeader() = delete;

    Slot _class;
};

// Important we not have a vtable in these objects, so no virtual functions.
static_assert(sizeof(ObjectHeader) == sizeof(Slot));

} // namespace hadron

#endif // SRC_HADRON_OBJECT_HEADER_HPP_