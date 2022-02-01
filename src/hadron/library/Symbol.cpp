#include "hadron/library/Symbol.hpp"

#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

namespace hadron {
namespace library {

// static
Symbol Symbol::fromHash(ThreadContext* context, Hash h) {
    assert(context->symbolTable->isDefined(h));
    return Symbol(h);
}

// static
Symbol Symbol::fromView(ThreadContext* context, std::string_view v) {
    return Symbol(context->symbolTable->addSymbol(context, v));
}

std::string_view Symbol::view(ThreadContext* context) const {
    return context->symbolTable->getString(*this).view();
}

} // namespace library
} // namespace hadron