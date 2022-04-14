#ifndef SRC_HADRON_HIR_PHI_HIR_HPP_
#define SRC_HADRON_HIR_PHI_HIR_HPP_

#include "hadron/hir/HIR.hpp"

#include <vector>

namespace hadron {

namespace lir {
struct PhiLIR;
}

namespace hir {

struct PhiHIR : public HIR {
    PhiHIR();
    explicit PhiHIR(library::Symbol n);
    virtual ~PhiHIR() = default;

    library::Symbol name;
    std::vector<ID> inputs;
    bool isSelfReferential;

    void addInput(HIR* input);
    // A phi is *trivial* if it has only one distinct input value that is not self-referential. If this phi is trivial,
    // return the trivial value. Otherwise return an invalid value.
    ID getTrivialValue() const;

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(LinearFrame* linearFrame) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_PHI_HIR_HPP_