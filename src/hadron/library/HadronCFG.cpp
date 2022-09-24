#include "hadron/library/HadronCFG.hpp"

#include "hadron/library/HadronHIR.hpp"

namespace hadron { namespace library {

HIRId CFGBlock::append(ThreadContext* context, HIR hir) {
    // Re-use constants with the same values.
    if (hir.className() == ConstantHIR::nameHash()) {
        // We're possibly skipping dependency updates for this constant, so ensure that constants never have value
        // dependencies.
        assert(hir.reads().size() == 0);
        auto constantHIR = ConstantHIR(hir.slot());
        // We can't use nil as a key in the constantValues() dictionary, so we save any nil constant value separately.
        if (constantHIR.constant()) {
            auto constantId = constantValues().typedGet(constantHIR.constant());
            if (constantId) {
                return constantId;
            }
        } else {
            if (nilConstantValue()) {
                return nilConstantValue();
            }
        }
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

    if (hir.className() == PhiHIR::nameHash()) {
        auto phiHIR = PhiHIR(hir.slot());
        setPhis(phis().typedAdd(context, phiHIR));
        return id;
    }

    // Adding a new constant, update the constants map and set.
    if (hir.className() == ConstantHIR::nameHash()) {
        auto constantHIR = ConstantHIR(hir.slot());
        if (constantHIR.constant()) {
            constantValues().typedPut(context, constantHIR.constant(), id);
            constantIds().typedAdd(context, id);
        } else {
            setNilConstantValue(id);
        }
    } else if (hir.className() == MethodReturnHIR::nameHash()) {
        setHasMethodReturn(true);
    }

    setStatements(statements().typedAdd(context, hir));

    return id;
}

} // namespace library
} // namespace hadron
