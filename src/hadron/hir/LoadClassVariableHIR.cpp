#include "hadron/hir/LoadClassVariableHIR.hpp"

namespace hadron {
namespace hir {

LoadClassVariableHIR::LoadClassVariableHIR(int index, library::Symbol name):
    HIR(kLoadClassVariable, Type::kAny, name),
    variableIndex(index) {}

NVID LoadClassVariableHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}


} // namespace hir
} // namespace hadron