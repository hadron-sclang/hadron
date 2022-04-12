#include "hadron/hir/WriteToClassHIR.hpp"

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

void WriteToClassHIR::lower(const std::vector<HIR*>& /* values */, LinearFrame* /* linearFrame */) const {
    assert(false);
}

} // namespace hir
} // namespace hadron
