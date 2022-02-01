#ifndef SRC_HADRON_SYMBOL_TABLE_HPP_
#define SRC_HADRON_SYMBOL_TABLE_HPP_

#include "hadron/Hash.hpp"
#include "hadron/library/String.hpp"
#include "hadron/library/Symbol.hpp"

#include <unordered_map>

namespace hadron {

class SymbolTable {
public:
    SymbolTable() = default;
    ~SymbolTable() = default;

    Hash addSymbol(ThreadContext* context, std::string_view v);

    // Returns true if the hash exists in the symbolMap.
    bool isDefined(Hash h) const { return m_symbolMap.find(h) != m_symbolMap.end(); }

    // String can be nil if hash not found.
    library::String getString(const library::Symbol s);

private:
    std::unordered_map<Hash, library::String> m_symbolMap;
};

} // namespace hadron

#endif // SRC_HADRON_SYMBOL_TABLE_HPP_