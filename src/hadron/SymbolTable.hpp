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
    Hash addSymbol(library::String s);

    // Returns true if the hash exists in the symbolMap.
    bool isDefined(Hash h) const { return m_symbolMap.find(h) != m_symbolMap.end(); }

    // String can be nil if hash not found.
    library::String getString(const library::Symbol s) const;

    std::string_view lookup(Hash h) const;

    // 'add'
    inline library::Symbol addSymbol() const { return m_add; }
    // 'Array'
    inline library::Symbol arraySymbol() const { return m_Array; }
    // 'at'
    inline library::Symbol atSymbol() const { return m_at; }
    // 'Class'
    inline library::Symbol classSymbol() const { return m_Class; }
    // 'classvar'
    inline library::Symbol classvarSymbol() const { return m_classvar; }
    // 'const'
    inline library::Symbol constSymbol() const { return m_const; }
    // 'copySeries'
    inline library::Symbol copySeriesSymbol() const { return m_copySeries; }
    // 'currentEnvironment'
    inline library::Symbol currentEnvironmentSymbol() const { return m_currentEnvironment; }
    // 'Event'
    inline library::Symbol eventSymbol() const { return m_Event; }
    // 'Function'
    inline library::Symbol functionSymbol() const { return m_Function; }
    // 'functionCompileContext'
    inline library::Symbol functionCompileContextSymbol() const { return m_functionCompileContext; }
    // 'Int8Array'
    inline library::Symbol int8ArraySymbol() const { return m_Int8Array; }
    // 'Interpreter'
    inline library::Symbol interpreterSymbol() const { return m_Interpreter; }
    // 'isNil'
    inline library::Symbol isNilSymbol() const { return m_isNil; }
    // 'Meta_Class'
    inline library::Symbol metaClassSymbol() const { return m_Meta_Class; }
    // 'new'
    inline library::Symbol newSymbol() const { return m_new; }
    // 'Object'
    inline library::Symbol objectSymbol() const { return m_Object; }
    // 'performList'
    inline library::Symbol performListSymbol() const { return m_performList; }
    // 'put'
    inline library::Symbol putSymbol() const { return m_put; }
    // 'super'
    inline library::Symbol superSymbol() const { return m_super; }
    // 'this'
    inline library::Symbol thisSymbol() const { return m_this; }
    // 'thisFunction'
    inline library::Symbol thisFunctionSymbol() const { return m_thisFunction; }
    // 'thisFunctionDef'
    inline library::Symbol thisFunctionDefSymbol() const { return m_thisFunctionDef; }
    // 'thisMethod'
    inline library::Symbol thisMethodSymbol() const { return m_thisMethod; }
    // 'thisProcess'
    inline library::Symbol thisProcessSymbol() const { return m_thisProcess; }
    // 'thisThread'
    inline library::Symbol thisThreadSymbol() const { return m_thisThread; }
    // 'value'
    inline library::Symbol valueSymbol() const { return m_value; }
    // 'var'
    inline library::Symbol varSymbol() const { return m_var; }
    // 'with'
    inline library::Symbol withSymbol() const { return m_with; }

private:
    std::unordered_map<Hash, library::String> m_symbolMap;

    library::Symbol m_add;
    library::Symbol m_Array;
    library::Symbol m_at;
    library::Symbol m_Class;
    library::Symbol m_classvar;
    library::Symbol m_const;
    library::Symbol m_copySeries;
    library::Symbol m_currentEnvironment;
    library::Symbol m_Event;
    library::Symbol m_Function;
    library::Symbol m_functionCompileContext;
    library::Symbol m_Int8Array;
    library::Symbol m_Interpreter;
    library::Symbol m_isNil;
    library::Symbol m_Meta_Class;
    library::Symbol m_new;
    library::Symbol m_Object;
    library::Symbol m_performList;
    library::Symbol m_put;
    library::Symbol m_super;
    library::Symbol m_this;
    library::Symbol m_thisFunction;
    library::Symbol m_thisFunctionDef;
    library::Symbol m_thisMethod;
    library::Symbol m_thisProcess;
    library::Symbol m_thisThread;
    library::Symbol m_value;
    library::Symbol m_var;
    library::Symbol m_with;
};

} // namespace hadron

#endif // SRC_HADRON_SYMBOL_TABLE_HPP_