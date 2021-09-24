#ifndef SRC_COMPILER_INCLUDE_HADRON_SCHEMA_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_SCHEMA_HPP_

#include "hadron/Slot.hpp"

namespace hadron {

// A Schema describes all the metatdata needed for the runtime to construct a SuperCollider object. Schemas can be both
// assembled by the runtime or pre-generated for consumption by the compiler. In the compiler-constructed case Hadron
// has the requirement that any class containing a primitive must be pre-processed by the compiler. This ensures that
// the compiler can enforce a consistent binding between primitive on the SCLang side and Schema member functions, and
// that assumptions about layouts of objects in memory are also validated.

// GOAL: stuff is either known at compile-time (e.g. constexpr) and is built by schema, OR is loaded into the SC Heap
// by the runtime. Trying to avoid a lot of stuff in the binary that then has to be potentially duplicated into the SC
// Heap (like string names of things, por ejemplo)

struct Schema {
    Hash nameSymbol;
};

}

#endif // SRC_COMPILER_INCLUDE_HADRON_SCHEMA_HPP_