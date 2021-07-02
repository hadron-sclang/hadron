#ifndef SRC_VALUE_HPP_
#define SRC_VALUE_HPP_

#include "Type.hpp"

#include <string>

namespace hadron {

struct Scope;

// Values live in a *Scope*, originally built in an AST (but possibly transferable to CFG/HIR structures??)
// Values can identify:
//   arguments
//   block variables
//   global variables
//   temporary (anonymous) variables
// essentially anything that can be *assigned to* and/or *read from*

// Value should carry the canonical "hard" copy of the name (a std::string)

struct Value {
    std::string name;
    Type type;
};

struct ValueRef {
    Value* value;
};

// We'll assume everything with known integer, boolean, float type is in a virtual register
// All "slot" variables live in memory

// The first task is name resolution -  find the scope/context in which the name "counter" is defined, for example.
// There's a well defined search order for name resolution, which needs research/definition and then testing.

/*
struct ValueRef {
    ValueRef(Block* b, uint64_t hash, std::string_view n): block(b), nameHash(hash), name(n),
        isBlockValue(false) {}
    explicit ValueRef(Block* b): block(b), nameHash(0), isBlockValue(true) {}
    Block* block;
    uint64_t nameHash;
    std::string_view name;
    bool isBlockValue;  // if true the nameHash and name are undefined.
};

struct Value {
    Value(): type(Type::kNil) {}
    Value(Type t, std::string_view n): type(t), name(n) {}
    Type type;
    std::string_view name;
};
*/

} // namespace hadron

#endif // SRC_VALUE_HPP_