#include "hadron/hir/RouteToSuperclassHIR.hpp"

namespace hadron {
namespace hir {

RouteToSuperclassHIR::RouteToSuperclassHIR(ID tID):
    HIR(kRouteToSuperclass),
    thisId(tID) {
    reads.emplace(tID);
}


ID RouteToSuperclassHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool RouteToSuperclassHIR::replaceInput(ID original, ID replacement) {
    if (replaceReads(original, replacement)) {
        assert(thisId == original);
        thisId = replacement;
        return true;
    }

    return false;
}

void RouteToSuperclassHIR::lower(const std::vector<HIR*>& /* values */, LinearFrame* /* linearFrame */) const {
    assert(false);
}

} // namespace hir
} // namespace hadron