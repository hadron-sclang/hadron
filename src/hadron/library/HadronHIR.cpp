#include "hadron/library/HadronHIR.hpp"
#include "hadron/library/HadronLIR.hpp"

namespace hadron {
namespace library {

HIRId HIR::proposeId(HIRId proposedId) {
    assert(proposedId && proposedId.int32() >= 0);

    switch (className()) {
    case BlockLiteralHIR::nameHash():
    case ConstantHIR::nameHash():
    case LoadOuterFrameHIR::nameHash():
    case MessageHIR::nameHash():
    case PhiHIR::nameHash():
    case ReadFromClassHIR::nameHash():
    case ReadFromContextHIR::nameHash():
    case ReadFromFrameHIR::nameHash():
    case ReadFromThisHIR::nameHash():
    case RouteToSuperclassHIR::nameHash():
        setId(proposedId);
        return proposedId;

    // Read-only HIR doesn't accept a value.
    case BranchHIR::nameHash():
    case BranchIfTrueHIR::nameHash():
    case MethodReturnHIR::nameHash():
    case StoreReturnHIR::nameHash():
    case WriteToClassHIR::nameHash():
    case WriteToFrameHIR::nameHash():
    case WriteToThisHIR::nameHash():
        setId(HIRId());
        return HIRId();
    }

    // Likely missing a type from the switch statement above.
    assert(false);
    return HIRId();
}

void PhiHIR::addInput(ThreadContext* context, HIR input) {
    assert(input.id());

    setInputs(inputs().typedAdd(context, input.id()));

    if (input.id() != id()) {
        reads().typedAdd(context, input.id());
        setTypeFlags(static_cast<TypeFlags>(static_cast<int32_t>(typeFlags()) |
                                            static_cast<int32_t>(input.typeFlags())));

        // This PhiHIR needs its own ID set by CFGBlock before any inputs are added. However, the CFGBlock append also
        // updates the consumers for each HIR. So we have to update consumers manually here on the Phi. It might be a
        // sign that the consumer updating should happen somewhere else, which is worth considering.
        input.consumers().typedAdd(context, toBase());
    } else {
        setIsSelfReferential(true);
    }
}

HIRId PhiHIR::getTrivialValue() const {
    // More than one distinct value in reads (which does not allow self-referential values) means this phi is
    // non-trivial.
    if (reads().size() > 1) {
        return HIRId();
    }

    // Phis with no inputs are invalid.
    assert(reads().size() == 1);
    return reads().typedNext(Slot::makeNil());
}

} // namespace library
} // namespace hadron