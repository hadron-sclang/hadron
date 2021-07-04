#ifndef SRC_HASH_HPP_
#define SRC_HASH_HPP_

#include <string_view>

namespace hadron {

// Changing this type or the underlying hashing algorithm will also require changing the hashkw.cpp utility to
// reflect.
using Hash = uint64_t;

Hash hash(std::string_view symbol);
Hash hash(const char* symbol, size_t length);

}  // hadron;

#endif // SRC_HASH_HPP_