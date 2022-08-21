#include "hadron/library/HadronLIR.hpp"

#include "hadron/JIT.hpp"
#include "hadron/ThreadContext.hpp"

namespace hadron {
namespace library {

bool LIR::producesValue() const {
    switch (className()) {
    case AssignLIR::nameHash():
    case LoadConstantLIR::nameHash():
    case LoadFromPointerLIR::nameHash():
    case PhiLIR::nameHash():
    case RemoveTagLIR::nameHash():
        return true;

    case BranchIfTrueLIR::nameHash():
    case BranchLIR::nameHash():
    case BranchToRegisterLIR::nameHash():
    case InterruptLIR::nameHash():
    case LabelLIR::nameHash():
    case PopFrameLIR::nameHash():
    case StoreToPointerLIR::nameHash():
        return false;
    }

    // Likely missing a type from the switch statement above.
    assert(false);
    return false;
}

bool LIR::shouldPreserveRegisters() const {
    return (className() == InterruptLIR::nameHash());
}

} // namespace library
} // namespace hadron