#include "hadron/hir/RouteToSuperclassHIR.hpp"

namespace hadron {
namespace hir {

RouteToSuperclassHIR::RouteToSuperclassHIR(NVID tID):
    HIR(kRouteToSuperclass),
    thisId(tID) {
    reads.emplace(tID);
}


NVID RouteToSuperclassHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

bool RouteToSuperclassHIR::replaceInput(NVID original, NVID replacement) {
    if (replaceReads(original, replacement)) {
        assert(thisId == original);
        thisId = replacement;
        return true;
    }

    return false;
}

void RouteToSuperclassHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    assert(false);
}

} // namespace hir
} // namespace hadron