#include "hadron/hir/RouteToSuperclassHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/AssignLIR.hpp"

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

void RouteToSuperclassHIR::lower(LinearFrame* linearFrame) const {
    // TODO: needs redesign
    auto thisVReg = linearFrame->hirToReg(thisId);
    linearFrame->append(id, std::make_unique<lir::AssignLIR>(thisVReg));
}

} // namespace hir
} // namespace hadron