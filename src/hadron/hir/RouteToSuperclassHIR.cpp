#include "hadron/hir/RouteToSuperclassHIR.hpp"

namespace hadron {
namespace hir {

RouteToSuperclassHIR::RouteToSuperclassHIR(NVID tID):
    HIR(kRouteToSuperclass),
    thisId(tID) {}


NVID RouteToSuperclassHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

void RouteToSuperclassHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    assert(false);
}

} // namespace hir
} // namespace hadron