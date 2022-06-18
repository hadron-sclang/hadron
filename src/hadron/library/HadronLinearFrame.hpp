#ifndef SRC_HADRON_LIBRARY_HADRON_LINEAR_FRAME_HPP_
#define SRC_HADRON_LIBRARY_HADRON_LINEAR_FRAME_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/Boolean.hpp"
#include "hadron/library/HadronLIR.hpp"
#include "hadron/library/Set.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/schema/HLang/HadronLinearFrameSchema.hpp"

namespace hadron {
namespace library {

class LiveRange : public Object<LiveRange, schema::HadronLiveRangeSchema> {
public:
    LiveRange(): Object<LiveRange, schema::HadronLiveRangeSchema>() {}
    explicit LiveRange(schema::HadronLiveRangeSchema* instance):
            Object<LiveRange, schema::HadronLiveRangeSchema>(instance) {}
    explicit LiveRange(Slot instance): Object<LiveRange, schema::HadronLiveRangeSchema>(instance) {}
    ~LiveRange() {}

    Integer from() const { return Integer(m_instance->from); }
    void setFrom(Integer f) { m_instance->from = f.slot(); }

    Integer to() const { return Integer(m_instance->to); }
    void setTo(Integer t) { m_instance->to = t.slot(); }
};

class LifetimeInterval : public Object<LifetimeInterval, schema::HadronLifetimeIntervalSchema> {
public:
    LifetimeInterval(): Object<LifetimeInterval, schema::HadronLifetimeIntervalSchema>() {}
    explicit LifetimeInterval(schema::HadronLifetimeIntervalSchema* instance):
            Object<LifetimeInterval, schema::HadronLifetimeIntervalSchema>(instance) {}
    explicit LifetimeInterval(Slot instance):
            Object<LifetimeInterval, schema::HadronLifetimeIntervalSchema>(instance) {}
    ~LifetimeInterval() {}

    TypedArray<LiveRange> ranges() const { return TypedArray<LiveRange>(m_instance->ranges); }
    void setRanges(TypedArray<LiveRange> r) { m_instance->ranges = r.slot(); }

    TypedIdentSet<Integer> usages() const { return TypedIdentSet<Integer>(m_instance->usages); }
    void setUsages(TypedIdentSet<Integer> u) { m_instance->usages = u.slot(); }

    VReg valueNumber() const { return VReg(m_instance->valueNumber); }
    void setValueNumber(VReg v) { m_instance->valueNumber = v.slot(); }

    Integer registerNumber() const { return Integer(m_instance->registerNumber); }
    void setRegisterNumber(Integer r) { m_instance->registerNumber = r.slot(); }

    Boolean isSplit() const { return Boolean(m_instance->isSplit); }
    void setIsSplit(Boolean b) { m_instance->isSplit = b.slot(); }

    Boolean isSpill() const { return Boolean(m_instance->isSpill); }
    void setIsSpill(Boolean b) { m_instance->isSpill = b.slot(); }

    Integer spillSlot() const { return Integer(m_instance->spillSlot); }
    void setSpillSlot(Integer i) { m_instance->spillSlot = i.slot(); }
};

class LinearFrame : public Object<LinearFrame, schema::HadronLinearFrameSchema> {
public:
    LinearFrame(): Object<LinearFrame, schema::HadronLinearFrameSchema>() {}
    explicit LinearFrame(schema::HadronLinearFrameSchema* instance):
            Object<LinearFrame, schema::HadronLinearFrameSchema>(instance) {}
    explicit LinearFrame(Slot instance): Object<LinearFrame, schema::HadronLinearFrameSchema>(instance) {}
    ~LinearFrame() {}

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

    TypedArray<LifetimeInterval> valueLifetimes() const {
        return TypedArray<LifetimeInterval>(m_instance->valueLifetimes);
    }
    void setValueLifetimes(TypedArray<LifetimeInterval> vL) { m_instance->valueLifetimes = vL.slot(); }

    Integer numberOfSpillSlots() const { return Integer(m_instance->numberOfSpillSlots); }
    void setNumberOfSpillSlots(Integer i) { m_instance->numberOfSpillSlots = i.slot(); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_LINEAR_FRAME_HPP_