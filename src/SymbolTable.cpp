#include "SymbolTable.hpp"

#include "xxhash.h"

namespace hadron {

// static
uint64_t SymbolTable::hash(std::string_view symbol) {
    return XXH3_64bits(symbol.data(), symbol.size());
}

// static
uint64_t SymbolTable::hash(const char* s, size_t length) {
    return XXH3_64bits(s, length);
}

}
