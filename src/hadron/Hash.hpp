#ifndef SRC_COMPILER_INCLUDE_HADRON_HASH_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_HASH_HPP_

#include <string_view>

namespace hadron {

using Hash = std::uint32_t;

Hash hash(std::string_view symbol, Hash seed = 0);
Hash hash(const char* symbol, size_t length, Hash seed = 0);

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_HASH_HPP_