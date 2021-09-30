#ifndef SRC_RUNTIME_OBJECT_HEADER_HPP_
#define SRC_RUNTIME_OBJECT_HEADER_HPP_

#include "hadron/Slot.hpp"

namespace runtime {

// Object instances in Hadron are contiguous blocks of Slots. The root SC Object has no instance variables accessible
// from the language, but descendent objects that have instance variables are appended on in declaration order. Objects
// with primitives are precompiled as structs that give their names to member variables and wrap some C++ around the
// runtime objects.
struct ObjectHeader {
    hadron::Slot _class;
};

} // namespace runtime

#endif // SRC_RUNTIME_OBJECT_HEADER_HPP_