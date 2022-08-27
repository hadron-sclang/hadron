#include "hadron/SymbolTable.hpp"

namespace hadron {

void SymbolTable::preloadSymbols(ThreadContext* context) {
    m_add = library::Symbol::fromView(context, "add");
    m_Array = library::Symbol::fromView(context, "Array");
    m_at = library::Symbol::fromView(context, "at");
    m_classvar = library::Symbol::fromView(context, "classvar");
    m_const = library::Symbol::fromView(context, "const");
    m_copySeries = library::Symbol::fromView(context, "copySeries");
    m_currentEnvironment = library::Symbol::fromView(context, "currentEnvironment");
    m_Event = library::Symbol::fromView(context, "Event");
    m_Function = library::Symbol::fromView(context, "Function");
    m_functionCompileContext = library::Symbol::fromView(context, "functionCompileContext");
    m_Interpreter = library::Symbol::fromView(context, "Interpreter");
    m_isNil = library::Symbol::fromView(context, "isNil");
    m_new = library::Symbol::fromView(context, "new");
    m_Object = library::Symbol::fromView(context, "Object");
    m_performList = library::Symbol::fromView(context, "performList");
    m_put = library::Symbol::fromView(context, "put");
    m_super = library::Symbol::fromView(context, "super");
    m_this = library::Symbol::fromView(context, "this");
    m_thisFunction = library::Symbol::fromView(context, "thisFunction");
    m_thisFunctionDef = library::Symbol::fromView(context, "thisFunctionDef");
    m_thisMethod = library::Symbol::fromView(context, "thisMethod");
    m_thisProcess = library::Symbol::fromView(context, "thisProcess");
    m_thisThread = library::Symbol::fromView(context, "thisThread");
    m_value = library::Symbol::fromView(context, "value");
    m_var = library::Symbol::fromView(context, "var");
    m_with = library::Symbol::fromView(context, "with");
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

Hash SymbolTable::addSymbol(library::String s) {
    Hash h = hash(s.view());
    auto mapIter = m_symbolMap.find(h);
    if (mapIter == m_symbolMap.end()) {
        m_symbolMap.emplace(std::make_pair(h, s));
    } else {
        assert(mapIter->second.compare(s));
    }
    return h;
}

library::String SymbolTable::getString(library::Symbol s) const {
    auto mapIter = m_symbolMap.find(s.hash());
    if (mapIter == m_symbolMap.end()) {
        assert(false);
        return library::String();
    }
    return mapIter->second;
}

std::string_view SymbolTable::lookup(Hash h) const {
    auto mapIter = m_symbolMap.find(h);
    if (mapIter == m_symbolMap.end()) {
        assert(false);
        return std::string_view();
    }
    return mapIter->second.view();
}

} // namespace hadron