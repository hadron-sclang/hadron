#ifndef SRC_COMPILER_INCLUDE_HADRON_HASH_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_HASH_HPP_

#include "hadron/Slot.hpp"

#include <functional>
#include <string_view>

namespace hadron {

// Changing this type or the underlying hashing algorithm will also require changing the hashkw.cpp utility to
// reflect.
using Hash = Slot;

Hash hash(std::string_view symbol);
Hash hash(const char* symbol, size_t length);

} // namespace hadron

namespace std {

template<>
struct hash<hadron::Slot> {
    size_t operator()(const hadron::Slot& s) const {
        return static_cast<size_t>(s.getHash());
    }
};

} // namespace std
#endif // SRC_COMPILER_INCLUDE_HADRON_HASH_HPP_