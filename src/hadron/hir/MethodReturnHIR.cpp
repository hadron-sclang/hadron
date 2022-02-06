#include "hadron/hir/MethodReturnHIR.hpp"

namespace hadron {
namespace hir {

MethodReturnHIR::MethodReturnHIR(NVID retVal):
        HIR(kMethodReturn), returnValue(retVal) {
    reads.emplace(retVal);
}

NVID MethodReturnHIR::proposeValue(NVID /* id */) {
    return kInvalidNVID;
}

} // namespace hir
} // namespace hadron