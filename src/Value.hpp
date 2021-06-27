#ifndef SRC_VALUE_HPP_
#define SRC_VALUE_HPP_

#include "Type.hpp"

#include <string_view>

namespace hadron {

struct Block;

struct ValueRef {
    ValueRef(Block* b, uint64_t hash, std::string_view n): block(b), nameHash(hash), name(n) {}
    Block* block;
    uint64_t nameHash;
    std::string_view name;
};

struct Value {
    Value(Type t, std::string_view n): type(t), name(n) {}
    Type type;
    std::string_view name;
};

} // namespace hadron

#endif // SRC_VALUE_HPP_