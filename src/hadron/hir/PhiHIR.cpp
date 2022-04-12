#include "hadron/hir/PhiHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/PhiLIR.hpp"

namespace hadron {
namespace hir {

PhiHIR::PhiHIR(): HIR(kPhi, TypeFlags::kNoFlags), isSelfReferential(false) {}

PhiHIR::PhiHIR(library::Symbol n): HIR(kPhi, TypeFlags::kNoFlags), name(n), isSelfReferential(false) {}

void PhiHIR::addInput(HIR* input) {
    assert(input->id != hir::kInvalidID);

    inputs.emplace_back(input->id);

    if (input->id != id) {
        reads.emplace(input->id);
        typeFlags = static_cast<TypeFlags>(static_cast<int32_t>(typeFlags) | static_cast<int32_t>(input->typeFlags));

        input->consumers.insert(static_cast<HIR*>(this));
    } else {
        isSelfReferential = true;
    }
}

ID PhiHIR::getTrivialValue() const {
    // More than one distinct value in reads (which does not allow self-referential values) means this phi is
    // non-trivial.
    if (reads.size() > 1) {
        return kInvalidID;
    }

    // Phis with no inputs are invalid.
    assert(reads.size() == 1);
    return *(reads.begin());
}

ID PhiHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool PhiHIR::replaceInput(ID original, ID replacement) {
    assert(original != replacement);

    bool inputsNeedSwap = false;
    if (replacement == id) {
        inputsNeedSwap = reads.erase(original);
        isSelfReferential = true;
    } else if (original == id) {
        if (isSelfReferential) {
            reads.emplace(replacement);
        }

        inputsNeedSwap = isSelfReferential;
        isSelfReferential = false;
    } else {
        inputsNeedSwap = replaceReads(original, replacement);
    }

    if (!inputsNeedSwap) { return false; }

    for (size_t i = 0; i < inputs.size(); ++i) {
        if (inputs[i] == original) {
            inputs[i] = replacement;
        }
    }

    return true;
}

void PhiHIR::lower(const std::vector<HIR*>& values, LinearFrame* linearFrame) const {
    auto phiLIR = std::make_unique<lir::PhiLIR>();

    for (auto nvid : inputs) {
        auto vReg = linearFrame->hirToReg(values[nvid]->id);
        assert(vReg != lir::kInvalidVReg);
        phiLIR->addInput(linearFrame->vRegs[vReg]->get());
    }

    linearFrame->append(id, std::move(phiLIR));
}

} // namespace hir
} // namespace hadron