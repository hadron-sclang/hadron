#ifndef SRC_SYMBOL_TABLE_HPP_
#define SRC_SYMBOL_TABLE_HPP_

#include <string>
#include <string_view>
#include <unordered_map>

namespace hadron {

// Maintains unique copies of symbols in scope. Symbols are identified by a 64-bit key.
class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable();

    // Adds a symbol by computing hash directly from memory, assuming there are no escape characters. Returns the
    // computed hash.
    uint64_t addSymbolVerbatim(std::string_view symbol);
    // Adds a symbol by processing escape characters first.
    uint64_t addSymbolEscaped(std::string_view symbol);
    // Hash a symbol only without adding it to the table.
    uint64_t hashOnly(std::string_view symbol) const;

    std::string_view getSymbol(uint64_t hash) const;

private:
    std::unordered_map<uint64_t, std::string> m_symbolMap;
};

} // namespace hadron

#endif // SRC_SYMBOL_TABLE_HPP_
