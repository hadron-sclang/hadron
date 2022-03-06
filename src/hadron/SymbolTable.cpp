#include "hadron/SymbolTable.hpp"

namespace hadron {

void SymbolTable::preloadSymbols(ThreadContext* context) {
    m_super = library::Symbol::fromView(context, "super");
    m_this = library::Symbol::fromView(context, "this");
    m_thisFunction = library::Symbol::fromView(context, "thisFunction");
    m_thisFunctionDef = library::Symbol::fromView(context, "thisFunctionDef");
    m_thisMethod = library::Symbol::fromView(context, "thisMethod");
    m_thisProcess = library::Symbol::fromView(context, "thisProcess");
    m_thisThread = library::Symbol::fromView(context, "thisThread");
}

Hash SymbolTable::addSymbol(ThreadContext* context, std::string_view v) {
    Hash h = hash(v);
    auto mapIter = m_symbolMap.find(h);
    if (mapIter == m_symbolMap.end()) {
        m_symbolMap.emplace(std::make_pair(h, library::String::fromView(context, v)));
    } else {
        // TODO: when this assert fails we have a hash collision and will need to design accordingly.
        assert(mapIter->second.compare(v));
    }
    return h;
}

library::String SymbolTable::getString(library::Symbol s) {
    auto mapIter = m_symbolMap.find(s.hash());
    if (mapIter == m_symbolMap.end()) {
        assert(false);
        return library::String();
    }
    return mapIter->second;
}

} // namespace hadron