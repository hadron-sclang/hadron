#ifndef SRC_HADRON_HIR_ROUTE_TO_SUPERCLASS_HIR_HPP_
#define SRC_HADRON_HIR_ROUTE_TO_SUPERCLASS_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct RouteToSuperclassHIR : public HIR {
    RouteToSuperclassHIR() = delete;
    explicit RouteToSuperclassHIR(NVID tID);
    virtual ~RouteToSuperclassHIR() = default;

    NVID thisId;

    NVID proposeValue(NVID id) override;
    bool replaceInput(NVID original, NVID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_ROUTE_TO_SUPERCLASS_HIR_HPP_