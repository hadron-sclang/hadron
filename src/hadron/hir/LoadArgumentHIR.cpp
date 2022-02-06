#include "hadron/hir/LoadArgumentHIR.hpp"

namespace hadron {
namespace hir {

LoadArgumentHIR::LoadArgumentHIR(int argIndex, library::Symbol name, bool varArgs = false):
        HIR(kLoadArgument, varArgs ? Type::kArray : Type::kAny, name),
        index(argIndex),
        isVarArgs(varArgs) {}

NamedValue LoadArgumentHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

} // namespace hir
} // namespace hadron