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

    // Computes the hash and stores the preloaded symbols, all of which are accessible via the get*() methods below.
    void preloadSymbols(ThreadContext* context);

    Hash addSymbol(ThreadContext* context, std::string_view v);

    // Returns true if the hash exists in the symbolMap.
    bool isDefined(Hash h) const { return m_symbolMap.find(h) != m_symbolMap.end(); }

    // String can be nil if hash not found.
    library::String getString(const library::Symbol s);

    library::Symbol superSymbol() const { return m_super; }
    library::Symbol thisSymbol() const { return m_this; }
    library::Symbol thisFunctionSymbol() const { return m_thisFunction; }
    library::Symbol thisFunctionDefSymbol() const { return m_thisFunctionDef; }
    library::Symbol thisMethodSymbol() const { return m_thisMethod; }
    library::Symbol thisProcessSymbol() const { return m_thisProcess; }
    library::Symbol thisThreadSymbol() const { return m_thisThread; }

private:
    std::unordered_map<Hash, library::String> m_symbolMap;

    library::Symbol m_super;
    library::Symbol m_this;
    library::Symbol m_thisFunction;
    library::Symbol m_thisFunctionDef;
    library::Symbol m_thisMethod;
    library::Symbol m_thisProcess;
    library::Symbol m_thisThread;
};

} // namespace hadron

#endif // SRC_HADRON_SYMBOL_TABLE_HPP_