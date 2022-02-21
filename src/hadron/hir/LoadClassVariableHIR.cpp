#include "hadron/hir/LoadClassVariableHIR.hpp"

namespace hadron {
namespace hir {

LoadClassVariableHIR::LoadClassVariableHIR(int index, library::Symbol name):
    HIR(kLoadClassVariable, TypeFlags::kAllFlags, name),
    variableIndex(index) {}

NVID LoadClassVariableHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}


} // namespace hir
} // namespace hadron