#include "hadron/library/Symbol.hpp"

#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

namespace hadron {
namespace library {

Symbol::Symbol(ThreadContext* context, Slot s): m_slot(s) {
    assert(s.isNil() || context->symbolTable->isDefined(s.getSymbolHash()));
}

// static
Symbol Symbol::fromView(ThreadContext* context, std::string_view v) {
    return Symbol(context->symbolTable->addSymbol(context, v));
}

// static
Symbol Symbol::fromString(ThreadContext* context, library::String s) {
    return Symbol(context->symbolTable->addSymbol(s));
}

std::string_view Symbol::view(ThreadContext* context) const {
    return context->symbolTable->getString(*this).view();
}

bool Symbol::isClassName(ThreadContext* context) const {
    if (m_slot.isNil()) { return false; }
    std::string_view v = view(context);
    if (v.size() == 0) { return false; }
    const char* c = v.data();
    return ('A' <= *c) && (*c <= 'Z');
}

} // namespace library
} // namespace hadron