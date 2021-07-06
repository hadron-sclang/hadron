#include "hadron/Hash.hpp"

#include "xxhash.h"

namespace hadron {

Hash hash(std::string_view symbol) {
    return XXH3_64bits(symbol.data(), symbol.size());
}

Hash hash(const char* symbol, size_t length) {
    return XXH3_64bits(symbol, length);
}

} // namespace hadron