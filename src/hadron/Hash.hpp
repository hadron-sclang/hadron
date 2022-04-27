#ifndef SRC_COMPILER_INCLUDE_HADRON_HASH_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_HASH_HPP_

#include "hadron/Slot.hpp"

#include <string_view>

namespace hadron {

Hash hash(std::string_view symbol);
Hash hash(const char* symbol, size_t length);

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_HASH_HPP_