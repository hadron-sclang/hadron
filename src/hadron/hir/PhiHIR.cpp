#include "hadron/hir/PhiHIR.hpp"

#include "hadron/lir/PhiLIR.hpp"

namespace hadron {
namespace hir {

PhiHIR::PhiHIR(): HIR(kPhi) {}

void PhiHIR::addInput(const HIR* input) {
    assert(input->value.id != hir::kInvalidNVID);
    inputs.emplace_back(input->value.id);
    reads.emplace(input->value.id);
    value.typeFlags = static_cast<TypeFlags>(static_cast<int32_t>(value.typeFlags) |
                                             static_cast<int32_t>(input->value.typeFlags));

    // Our knownClassName should accept the value of the first input added to the phi, and only keep that value if it
    // matches every other input.
    if (value.typeFlags & TypeFlags::kObjectFlag) {
        if (inputs.size() == 1) {
            value.knownClassName = input->value.knownClassName;
        } else if (value.knownClassName != input->value.knownClassName) {
            value.knownClassName = library::Symbol();
        }
    } else {
        value.knownClassName = library::Symbol();
    }
}

NVID PhiHIR::getTrivialValue() const {
    // More than two distinct values means that even if one of them is self-referential this phi has two or more
    // non self-referential distinct values and is therefore nontrivial.
    if (reads.size() > 2) {
        return kInvalidNVID;
    }

    // Exactly two distinct values means that if either of the two values are self-referential than the phi is trivial
    // and we should return the other non-self-referential value.
    if (reads.size() == 2) {
        NVID nonSelf;
        bool trivial = false;
        for (auto v : reads) {
            if (v != value.id) {
                nonSelf = v;
            } else {
                trivial = true;
            }
        }
        if (trivial) {
            return nonSelf;
        } else {
            // Two unique values that aren't self means this is a nontrivial phi.
            return kInvalidNVID;
        }
    }

    // Phis with no inputs are invalid.
    assert(reads.size() == 1);
    return *(reads.begin());
}

NVID PhiHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

void PhiHIR::lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const {
    auto phiLIROwning = std::make_unique<lir::PhiLIR>(vReg());
    lir::PhiLIR* phiLIR = phiLIROwning.get();
    append.emplace_back(std::move(phiLIROwning));
    vRegs[vReg()] = --(append.end());

    for (auto nvid : inputs) {
        phiLIR->addInput(values[nvid]->vReg(), vRegs);
    }
}

} // namespace hir
} // namespace hadron