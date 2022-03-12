#include "hadron/hir/PhiHIR.hpp"

#include "hadron/lir/PhiLIR.hpp"

namespace hadron {
namespace hir {

PhiHIR::PhiHIR(): HIR(kPhi) {}

PhiHIR::PhiHIR(library::Symbol name): HIR(kPhi, TypeFlags::kNoFlags, name), isSelfReferential(false) {}

void PhiHIR::addInput(HIR* input) {
    assert(input->value.id != hir::kInvalidNVID);

    inputs.emplace_back(input->value.id);

    if (input->value.id != value.id) {
        reads.emplace(input->value.id);
        value.typeFlags = static_cast<TypeFlags>(static_cast<int32_t>(value.typeFlags) |
                                                 static_cast<int32_t>(input->value.typeFlags));

        input->consumers.insert(static_cast<HIR*>(this));

        // Our knownClassName should accept the value of the first input added to the phi, and only keep that value if
        // it matches every other input.
        if (value.typeFlags & TypeFlags::kObjectFlag) {
            if (inputs.size() == 1) {
                value.knownClassName = input->value.knownClassName;
            } else if (value.knownClassName != input->value.knownClassName) {
                value.knownClassName = library::Symbol();
            }
        } else {
            value.knownClassName = library::Symbol();
        }
    } else {
        isSelfReferential = true;
    }
}

NVID PhiHIR::getTrivialValue() const {
    // More than one distinct value in reads (which does not allow self-referential values) means this phi is
    // non-trivial.
    if (reads.size() > 1) {
        return kInvalidNVID;
    }

    // Phis with no inputs are invalid.
    assert(reads.size() == 1);
    return *(reads.begin());
}

NVID PhiHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

bool PhiHIR::replaceInput(NVID original, NVID replacement) {
    assert(original != replacement);

    bool inputsNeedSwap = false;
    if (replacement == value.id) {
        inputsNeedSwap = reads.erase(original);
        isSelfReferential = true;
    } else if (original == value.id) {
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