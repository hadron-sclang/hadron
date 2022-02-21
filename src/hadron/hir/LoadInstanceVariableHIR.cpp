#include "hadron/hir/LoadInstanceVariableHIR.hpp"

namespace hadron {
namespace hir {

LoadInstanceVariableHIR::LoadInstanceVariableHIR(NVID instance, int index, library::Symbol variableName):
    HIR(kLoadInstanceVariable, TypeFlags::kAllFlags, variableName),
    instanceID(instance),
    variableIndex(index) {}

NVID LoadInstanceVariableHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

} // namespace hir
} // namespace hadron