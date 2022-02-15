#ifndef SRC_HADRON_HIR_PHI_HIR_HPP_
#define SRC_HADRON_HIR_PHI_HIR_HPP_

#include "hadron/hir/HIR.hpp"
#include "hadron/Type.hpp"

#include <vector>

namespace hadron {

namespace lir {
struct PhiLIR;
}

namespace hir {

struct PhiHIR : public HIR {
    PhiHIR();
    virtual ~PhiHIR() = default;

    std::vector<NVID> inputs;
    void addInput(const HIR* input);
    // A phi is *trivial* if it has only one distinct input value that is not self-referential. If this phi is trivial,
    // return the trivial value. Otherwise return an invalid value.
    NVID getTrivialValue() const;

    NVID proposeValue(NVID id) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_PHI_HIR_HPP_