#include "hadron/library/HadronLinearFrame.hpp"

namespace hadron {
namespace library {

VReg LinearFrame::append(ThreadContext* context, HIRId hirId, LIR lir, TypedArray<LIR>& lirArray) {
    VReg value;
    if (lir.producesValue()) {
        value = VReg(vRegs().size());
        lir.setVReg(value);
        setVRegs(vRegs().typedAdd(context, lir));
    }

    if (hirId) {
        // Every valid HIRId needs an associated valid vReg. The opposite is not necessarily true, meaning LIR can
        // produce vRegs that have no associated HIRId.
        assert(value);
        hirToRegMap().typedPut(context, hirId, value);
    }

    lirArray = lirArray.typedAdd(context, lir);

    return value;
}

} // namespace library
} // namespace hadron