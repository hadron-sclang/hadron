#include "SymbolTable.hpp"

#include "xxhash.h"

namespace hadron {

SymbolTable::SymbolTable() {}
SymbolTable::~SymbolTable() {}

uint64_t SymbolTable::addSymbolVerbatim(std::string_view symbol) {
    uint64_t hash = XXH3_64bits(symbol.data(), symbol.size());
    m_symbolMap.emplace(std::make_pair(hash, std::string(symbol)));
    return hash;
}

// TODO
uint64_t SymbolTable::addSymbolEscaped(std::string_view symbol) {
    uint64_t hash = XXH3_64bits(symbol.data(), symbol.size());
    m_symbolMap.emplace(std::make_pair(hash, std::string(symbol)));
    return hash;
}

std::string_view SymbolTable::getSymbol(uint64_t hash) const {
    auto it = m_symbolMap.find(hash);
    if (it != m_symbolMap.end()) {
        return std::string_view(it->second.data(), it->second.size());
    }
    return "";
}

}
