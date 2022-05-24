#include "hadron/Hash.hpp"

#include "xxhash.h"

#include <cassert>
#include <vector>

namespace hadron {

Hash hash(std::string_view symbol, Hash seed) {
    return hash(symbol.data(), symbol.size(), seed);
}

Hash hash(const char* symbol, size_t length, Hash seed) {
    return XXH32(symbol, length, seed);
}

} // namespace hadron