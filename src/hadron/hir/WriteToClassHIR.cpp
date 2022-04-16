#include "hadron/hir/WriteToClassHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/StoreToPointerLIR.hpp"

namespace hadron {
namespace hir {

WriteToClassHIR::WriteToClassHIR(hir::ID classArray, int32_t index, library::Symbol name, hir::ID v):
        HIR(kWriteToClass), classVariableArray(classArray), arrayIndex(index), valueName(name), toWrite(v) {
    reads.emplace(classVariableArray);
    reads.emplace(toWrite);
}

ID WriteToClassHIR::proposeValue(ID /* proposedId */) {
    return kInvalidID;
}

bool WriteToClassHIR::replaceInput(ID original, ID replacement) {
    if (replaceReads(original, replacement)) {
        if (classVariableArray == original) {
            classVariableArray = replacement;
        }
        if (toWrite == original) {
            toWrite = replacement;
        }
        return true;
    }

    return false;
}

void WriteToClassHIR::lower(LinearFrame* linearFrame) const {
    auto classVarVReg = linearFrame->hirToReg(classVariableArray);
    auto toWriteVReg = linearFrame->hirToReg(toWrite);
    linearFrame->append(kInvalidID, std::make_unique<lir::StoreToPointerLIR>(classVarVReg, toWriteVReg, arrayIndex));
}

} // namespace hir
} // namespace hadron
