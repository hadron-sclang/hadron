#ifndef SRC_COMPILER_INCLUDE_HADRON_HASH_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_HASH_HPP_

#include <string_view>

namespace hadron {

struct Signature;

// Changing this type or the underlying hashing algorithm will also require changing the hashkw.cpp utility to
// reflect.
using Hash = uint64_t;

Hash hash(std::string_view symbol);
Hash hash(const char* symbol, size_t length);

Hash hashSignature(Signature* sig);

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_HASH_HPP_