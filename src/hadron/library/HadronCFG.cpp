#include "hadron/library/HadronCFG.hpp"

#include "hadron/library/HadronHIR.hpp"

namespace hadron {
namespace library {

HIRId CFGBlock::append(ThreadContext* context, HIR hir) {
    assert(hir.className() != PhiHIR::nameHash());

    // Re-use constants with the same values.
    if (hir.className() == ConstantHIR::nameHash()) {
        // We're possibly skipping dependency updates for this constant, so ensure that constants never have value
        // dependencies.
        assert(hir.reads().size() == 0);
        auto constantHIR = ConstantHIR(hir.slot());
        auto constantId = constantValues().typedGet(constantHIR.constant());
        if (constantId) { return constantId; }
    }

    auto id = HIRId(frame().values().size());
    id = hir.proposeId(id);
    if (id) {
        frame().setValues(frame().values().typedAdd(context, hir));
    }

    hir.setOwningBlock(*this);

    // Update the producers of values this hir consumes.
    if (hir.reads().size()) {
        auto readsArray = hir.reads().typedArray();
        for (int32_t i = 0; i < readsArray.size(); ++i) {
            auto readsId = readsArray.typedAt(i);
            if (readsId) {
                frame().values().typedAt(readsId.int32()).consumers().typedAdd(context, hir);
            }
        }
    }

    bool addToExitStatements = false;

    // Adding a new constant, update the constants map and set.
    if (hir.className() == ConstantHIR::nameHash()) {
        auto constantHIR = ConstantHIR(hir.slot());
        constantValues.typedPut(context, constantHIR.constant(), id);
        constantIds.typedAdd(context, id);
    } else if (hir.className() == MethodReturnHIR::nameHash()) {
        setHasMethodReturn(true);
        addToExitStatements = true;
    } else if (hir.className() == BranchHIR::nameHash() || hir.className() == BranchIfTrueHIR::nameHash()) {
        addToExitStatements = true;
    }

    if (addToExitStatements) {
        setExitStatements(exitStatements().typedAdd(context, hir));
    } else {
        setStatements(statements().typedAdd(context, hir));
    }

    return HIRId();
}

} // namespace library
} // namespace hadron
