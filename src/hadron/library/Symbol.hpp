#ifndef SRC_HADRON_LIBRARY_SYMBOL_HPP_
#define SRC_HADRON_LIBRARY_SYMBOL_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"

#include <string_view>

namespace hadron {

struct ThreadContext;

namespace library {

class String;

// Rather than wrapping a pointer, a Symbol is a wrapper around a Hash of a String. The difference between Hash and
// Symbol is that we try to take care that a Symbol must always exist in the SymbolTable, and we provide some debug
// consistency checks that enforce that.
class Symbol {
public:
    // Make an empty symbol.
    Symbol(): m_slot(Slot::makeNil()) { }
    Symbol(const Symbol& s): m_slot(s.m_slot) { }
    Symbol& operator=(const Symbol& s) {
        m_slot = s.m_slot;
        return *this;
    }
    // Asserts that s is either nil or a valid hash in the symbol table.
    Symbol(ThreadContext* context, Slot s);
    // Asserts that h is a valid hash in the symbol table.
    Symbol(ThreadContext* context, Hash h);
    ~Symbol() { }

    // Creates a new symbol from the string_view |v|, if not already defined.
    static Symbol fromView(ThreadContext* context, std::string_view v);
    // Creates a new symbol from an already existing String, avoiding the copy.
    static Symbol fromString(ThreadContext* context, library::String s);

    inline bool isNil() const { return m_slot.isNil(); }
    inline Hash hash() const { return m_slot.getSymbolHash(); }
    inline Slot slot() const { return m_slot; }
    std::string_view view(ThreadContext* context) const;

    bool isClassName(ThreadContext* context) const;
    bool isMetaClassName(ThreadContext* context) const;

    inline bool operator==(const Symbol& s) const { return m_slot == s.m_slot; }
    inline bool operator!=(const Symbol& s) const { return m_slot != s.m_slot; }
    explicit inline operator bool() const { return !isNil(); }

private:
    Symbol(Hash h): m_slot(Slot::makeSymbol(h)) { }
    Slot m_slot;
};

} // namespace library
} // namespace hadron

namespace std {

template <> struct hash<hadron::library::Symbol> {
    size_t operator()(const hadron::library::Symbol& s) const { return static_cast<size_t>(s.hash()); }
};

} // namespace std

#endif // SRC_HADRON_LIBRARY_SYMBOL_HPP_