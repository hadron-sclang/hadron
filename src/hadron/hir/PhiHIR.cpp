#include "hadron/hir/PhiHIR.hpp"

namespace hadron {
namespace hir {

void PhiHIR::addInput(const HIR* input) {
    inputs.emplace_back(input->value.id);
    reads.emplace(input->value.id);
    value.typeFlags = static_cast<Type>(static_cast<int32_t>(value.typeFlags) |
                                        static_cast<int32_t>(input->value.typeFlags));
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
        }
    }

    // Phis with no values are invalid.
    assert(reads.size() == 1);
    return *(reads.begin());
}

NVID PhiHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

} // namespace hir
} // namespace hadron