#ifndef SRC_HADRON_HIR_LOAD_ARGUMENT_HIR_HPP_
#define SRC_HADRON_HIR_LOAD_ARGUMENT_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

// Loads the argument at |index| from the stack.
struct LoadArgumentHIR : public HIR {
    LoadArgumentHIR() = delete;
    explicit LoadArgumentHIR(int index);
    virtual ~LoadArgumentHIR() = default;

    int argIndex;

    // Forces the kAny type for all arguments.
    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_LOAD_ARGUMENT_HIR_HPP_