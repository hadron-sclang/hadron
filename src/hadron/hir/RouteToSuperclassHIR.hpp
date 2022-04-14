#ifndef SRC_HADRON_HIR_ROUTE_TO_SUPERCLASS_HIR_HPP_
#define SRC_HADRON_HIR_ROUTE_TO_SUPERCLASS_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct RouteToSuperclassHIR : public HIR {
    RouteToSuperclassHIR() = delete;
    explicit RouteToSuperclassHIR(ID tID);
    virtual ~RouteToSuperclassHIR() = default;

    ID thisId;

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(LinearFrame* linearFrame) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_ROUTE_TO_SUPERCLASS_HIR_HPP_