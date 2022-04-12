#include "hadron/hir/ReadFromClassHIR.hpp"

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

void ReadFromClassHIR::lower(const std::vector<HIR*>& /* values */, LinearFrame* /* linearFrame */) const {
    // WRITEME
    assert(false);
}

} // namespace hir
} // namespace hadron