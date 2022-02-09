#include "hadron/hir/LoadArgumentHIR.hpp"

#include "hadron/lir/LoadFromStackLIR.hpp"

namespace hadron {
namespace hir {

LoadArgumentHIR::LoadArgumentHIR(int argIndex, library::Symbol name, bool varArgs):
        HIR(kLoadArgument, varArgs ? Type::kArray : Type::kAny, name),
        index(argIndex),
        isVarArgs(varArgs) {}

NVID LoadArgumentHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

void LoadArgumentHIR::lower(const std::vector<HIR*>& /* values */,
        std::vector<std::unique_ptr<lir::LIR>>& append) const {
    append.emplace_back(std::make_unique<lir::LoadFromStackLIR>(vReg(), index));
}

} // namespace hir
} // namespace hadron