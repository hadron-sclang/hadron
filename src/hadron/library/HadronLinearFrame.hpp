#ifndef SRC_HADRON_LIBRARY_HADRON_LINEAR_FRAME_HPP_
#define SRC_HADRON_LIBRARY_HADRON_LINEAR_FRAME_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/Dictionary.hpp"
#include "hadron/library/HadronLIR.hpp"
#include "hadron/library/HadronLifetimeInterval.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/Set.hpp"
#include "hadron/schema/HLang/HadronLinearFrameSchema.hpp"

namespace hadron {
namespace library {

class LinearFrame : public Object<LinearFrame, schema::HadronLinearFrameSchema> {
public:
    LinearFrame(): Object<LinearFrame, schema::HadronLinearFrameSchema>() {}
    explicit LinearFrame(schema::HadronLinearFrameSchema* instance):
            Object<LinearFrame, schema::HadronLinearFrameSchema>(instance) {}
    explicit LinearFrame(Slot instance): Object<LinearFrame, schema::HadronLinearFrameSchema>(instance) {}
    ~LinearFrame() {}

    // Convenience function, returns associated VReg in LIR or nil if no hir value found.
    inline VReg hirToReg(HIRId hirId) { return hirToRegMap().typedGet(hirId); }

    // Appends lir to lirArray, which is assumed to be in the LinearBlock. If hirId is valid then we add to the
    // mapping. Returns the assigned VReg or kInvalidVReg if no value assigned. Note the pass-by-reference on the
    // |lirArray| argument, allowing the array to be re-assigned on additions.
    VReg append(ThreadContext* context, HIRId hirId, LIR lir, TypedArray<LIR>& lirArray);

    TypedArray<LIR> instructions() const { return TypedArray<LIR>(m_instance->instructions); }
    void setInstructions(TypedArray<LIR> inst) { m_instance->instructions = inst.slot(); }

    TypedArray<LIR> vRegs() const { return TypedArray<LIR>(m_instance->vRegs); }
    void setVRegs(TypedArray<LIR> v) { m_instance->vRegs = v.slot(); }

    TypedArray<LabelId> blockOrder() const { return TypedArray<LabelId>(m_instance->blockOrder); }
    void setBlockOrder(TypedArray<LabelId> bo) { m_instance->blockOrder = bo.slot(); }

    TypedArray<LabelLIR> blockLabels() const { return TypedArray<LabelLIR>(m_instance->blockLabels); }
    void setBlockLabels(TypedArray<LabelLIR> lbls) { m_instance->blockLabels = lbls.slot(); }

    TypedArray<LiveRange> blockRanges() const { return TypedArray<LiveRange>(m_instance->blockRanges); }
    void setBlockRanges(TypedArray<LiveRange> r) { m_instance->blockRanges = r.slot(); }

    TypedArray<TypedIdentSet<VReg>> liveIns() const { return TypedArray<TypedIdentSet<VReg>>(m_instance->liveIns); }
    void setLiveIns(TypedArray<TypedIdentSet<VReg>> li) { m_instance->liveIns = li.slot(); }

    using Intervals = TypedArray<TypedArray<LifetimeInterval>>;
    Intervals valueLifetimes() const { return Intervals(m_instance->valueLifetimes); }
    void setValueLifetimes(Intervals ivls) { m_instance->valueLifetimes = ivls.slot(); }

    Integer numberOfSpillSlots() const { return Integer(m_instance->numberOfSpillSlots); }
    void setNumberOfSpillSlots(Integer i) { m_instance->numberOfSpillSlots = i.slot(); }

    TypedIdentDict<HIRId, VReg> hirToRegMap() const { return TypedIdentDict<HIRId, VReg>(m_instance->hirToRegMap); }
    void setHIRToRegMap(TypedIdentDict<HIRId, VReg> a) { m_instance->hirToRegMap = a.slot(); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_LINEAR_FRAME_HPP_