#include "hadron/hir/ReadFromClassHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/LoadFromPointerLIR.hpp"

namespace hadron {
namespace hir {

ReadFromClassHIR::ReadFromClassHIR(hir::ID classArray, int index, library::Symbol name):
    HIR(kReadFromClass, TypeFlags::kAllFlags),
    classVariableArray(classArray),
    arrayIndex(index),
    valueName(name) { reads.emplace(classArray); }

ID ReadFromClassHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool ReadFromClassHIR::replaceInput(ID original, ID replacement) {
    if (replaceReads(original, replacement)) {
        assert(classVariableArray == original);
        classVariableArray = replacement;
        return true;
    }

    return false;
}

void ReadFromClassHIR::lower(LinearFrame* linearFrame) const {
    auto classVarVReg = linearFrame->hirToReg(classVariableArray);
    linearFrame->append(id, std::make_unique<lir::LoadFromPointerLIR>(classVarVReg, arrayIndex));
}

} // namespace hir
} // namespace hadron