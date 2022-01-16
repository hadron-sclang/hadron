#include "hadron/Hash.hpp"

#include "hadron/Slot.hpp"

#include "xxhash.h"

namespace hadron {

Hash hash(std::string_view symbol) {
    return Slot::makeHash(XXH3_64bits(symbol.data(), symbol.size())).getHash();
}

Hash hash(const char* symbol, size_t length) {
    return Slot::makeHash(XXH3_64bits(symbol, length)).getHash();
}

} // namespace hadron