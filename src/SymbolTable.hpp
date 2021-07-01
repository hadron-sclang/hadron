#ifndef SRC_SYMBOL_TABLE_HPP_
#define SRC_SYMBOL_TABLE_HPP_

#include <string_view>

namespace hadron {

// Maintains unique copies of symbols in scope. Symbols are identified by a 64-bit key.
class SymbolTable {
public:
    SymbolTable() = default;
    ~SymbolTable() = default;

    // Hashes a symbol, returning the value.
    static uint64_t hash(std::string_view symbol);
    static uint64_t hash(const char* s, size_t length);
};

} // namespace hadron

#endif // SRC_SYMBOL_TABLE_HPP_
