#ifndef SRC_HADRON_LIBRARY_HADRON_LIR_HPP_
#define SRC_HADRON_LIBRARY_HADRON_LIR_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/Dictionary.hpp"
#include "hadron/library/Integer.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/Set.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/schema/HLang/HadronLIRSchema.hpp"

namespace hadron {
namespace library {

using VReg = Integer;
static constexpr VReg kContextPointerVReg = VReg(-3);
static constexpr VReg kFramePointerVReg = VReg(-2);
static constexpr VReg kStackPointerVReg = VReg(-1);

template<typename T, typename S, typename B>
class LIRBase : public Object<T, S> {
public:
    LIRBase(): Object<T, S>() {}
    explicit LIRBase(S* instance): Object<T, S>(instance) {}
    explicit LIRBase(Slot instance): Object<T, S>(instance) {}
    ~LIRBase() {}

    B toBase() const {
        const T& t = static_cast<const T&>(*this);
        return B::wrapUnsafe(Slot::makePointer(reinterpret_cast<library::Schema*>(t.m_instance)));
    }

    static T make(ThreadContext* context) {
        auto t = T::alloc(context);
        t.initToNil();
        return t;
    }

    VReg vReg() const {
        const T& t = static_cast<const T&>(*this);
        return VReg(t.m_instance->vReg);
    }
    void setVReg(VReg v) {
        T& t = static_cast<T&>(*this);
        t.m_instance->vReg = v.slot();
    }

    TypeFlags typeFlags() const {
        const T& t = static_cast<const T&>(*this);
        return static_cast<TypeFlags>(t.m_instance->typeFlags.getInt32());
    }
    void setTypeFlags(TypeFlags f) {
        T& t = static_cast<T&>(*this);
        t.m_instance->typeFlags = Slot::makeInt32(f);
    }

    TypedIdentSet<VReg> reads() const {
        const T& t = static_cast<const T&>(*this);
        return TypedIdentSet<VReg>(t.m_instance->reads);
    }
    void setReads(TypedIdentSet<VReg> r) {
        T& t = static_cast<T&>(*this);
        t.m_instance->reads = r.slot();
    }

    TypedIdentDict<VReg, Reg> locations() const {
        const T& t = static_cast<const T&>(*this);
        return TypedIdentDict<VReg, Reg>(t.m_instance->locations);
    }
    void setLocations(TypedIdentDict<VReg, Reg> loc) {
        T& t = static_cast<T&>(*this);
        t.m_instance->locations = loc.slot();
    }

    TypedIdentDict<Integer, Integer> moves() const {
        const T& t = static_cast<const T&>(*this);
        return TypedIdentDict<Integer, Integer>(t.m_instance->moves);
    }
    void setMoves(TypedIdentDict<Integer, Integer> m) {
        T& t = static_cast<T&>(*this);
        t.m_instance->moves = m.slot();
    }
};

class LIR : public LIRBase<LIR, schema::HadronLIRSchema, LIR> {
public:
    LIR(): LIRBase<LIR, schema::HadronLIRSchema, LIR>() {}
    explicit LIR(schema::HadronLIRSchema* instance): LIRBase<LIR, schema::HadronLIRSchema, LIR>(instance) {}
    explicit LIR(Slot instance): LIRBase<LIR, schema::HadronLIRSchema, LIR>(instance) {}
    ~LIR() {}

    bool producesValue() const;
    bool shouldPreserveRegisters() const;
};

class AssignLIR : public LIRBase<AssignLIR, schema::HadronAssignLIRSchema, LIR> {
public:
    AssignLIR(): LIRBase<AssignLIR, schema::HadronAssignLIRSchema, LIR>() {}
    explicit AssignLIR(schema::HadronAssignLIRSchema* instance):
            LIRBase<AssignLIR, schema::HadronAssignLIRSchema, LIR>(instance) {}
    explicit AssignLIR(Slot instance):
            LIRBase<AssignLIR, schema::HadronAssignLIRSchema, LIR>(instance) {}
    ~AssignLIR() {}

    VReg origin() const { return VReg(m_instance->origin); }
    void setOrigin(VReg o) { m_instance->origin = o.slot(); }
};

class BranchLIR : public LIRBase<BranchLIR, schema::HadronBranchLIRSchema, LIR> {
public:
    BranchLIR(): LIRBase<BranchLIR, schema::HadronBranchLIRSchema, LIR>() {}
    explicit BranchLIR(schema::HadronBranchLIRSchema* instance):
            LIRBase<BranchLIR, schema::HadronBranchLIRSchema, LIR>(instance) {}
    explicit BranchLIR(Slot instance): LIRBase<BranchLIR, schema::HadronBranchLIRSchema, LIR>(instance) {}
    ~BranchLIR() {}

    LabelId labelId() const { return LabelId(m_instance->labelId); }
    void setLabelId(LabelId id) { m_instance->labelId = id.slot(); }
};

class BranchIfTrueLIR : public LIRBase<BranchIfTrueLIR, schema::HadronBranchIfTrueLIRSchema, LIR> {
public:
    BranchIfTrueLIR(): LIRBase<BranchIfTrueLIR, schema::HadronBranchIfTrueLIRSchema, LIR>() {}
    explicit BranchIfTrueLIR(schema::HadronBranchIfTrueLIRSchema* instance):
            LIRBase<BranchIfTrueLIR, schema::HadronBranchIfTrueLIRSchema, LIR>(instance) {}
    explicit BranchIfTrueLIR(Slot instance):
            LIRBase<BranchIfTrueLIR, schema::HadronBranchIfTrueLIRSchema, LIR>(instance) {}
    ~BranchIfTrueLIR() {}

    VReg condition() const { return VReg(m_instance->condition); }
    void setCondition(VReg c) { m_instance->condition = c.slot(); }

    LabelId labelId() const { return LabelId(m_instance->labelId); }
    void setLabelId(LabelId id) { m_instance->labelId = id.slot(); }
};

class BranchToRegisterLIR : public LIRBase<BranchToRegisterLIR, schema::HadronBranchToRegisterLIRSchema, LIR> {
public:
    BranchToRegisterLIR(): LIRBase<BranchToRegisterLIR, schema::HadronBranchToRegisterLIRSchema, LIR>() {}
    explicit BranchToRegisterLIR(schema::HadronBranchToRegisterLIRSchema* instance):
            LIRBase<BranchToRegisterLIR, schema::HadronBranchToRegisterLIRSchema, LIR>(instance) {}
    explicit BranchToRegisterLIR(Slot instance):
            LIRBase<BranchToRegisterLIR, schema::HadronBranchToRegisterLIRSchema, LIR>(instance) {}
    ~BranchToRegisterLIR() {}

    VReg address() const { return VReg(m_instance->address); }
    void setAddress(VReg addr) { m_instance->address = addr.slot(); }
};

class InterruptLIR : public LIRBase<InterruptLIR, schema::HadronInterruptLIRSchema, LIR> {
public:
    InterruptLIR(): LIRBase<InterruptLIR, schema::HadronInterruptLIRSchema, LIR>() {}
    explicit InterruptLIR(schema::HadronInterruptLIRSchema* instance):
            LIRBase<InterruptLIR, schema::HadronInterruptLIRSchema, LIR>(instance) {}
    explicit InterruptLIR(Slot instance): LIRBase<InterruptLIR, schema::HadronInterruptLIRSchema, LIR>(instance) {}
    ~InterruptLIR() {}

    Integer interruptCode() const { return Integer(m_instance->interruptCode); }
    void setInterruptCode(Integer code) { m_instance->interruptCode = code.slot(); }
};

class PhiLIR : public LIRBase<PhiLIR, schema::HadronPhiLIRSchema, LIR> {
public:
    PhiLIR(): LIRBase<PhiLIR, schema::HadronPhiLIRSchema, LIR>() {}
    explicit PhiLIR(schema::HadronPhiLIRSchema* instance):
            LIRBase<PhiLIR, schema::HadronPhiLIRSchema, LIR>(instance) {}
    explicit PhiLIR(Slot instance):
            LIRBase<PhiLIR, schema::HadronPhiLIRSchema, LIR>(instance) {}
    ~PhiLIR() {}

    TypedArray<VReg> inputs() const { return TypedArray<VReg>(m_instance->inputs); }
    void setInputs(TypedArray<VReg> i) { m_instance->inputs = i.slot(); }

    inline void addInput(ThreadContext* context, LIR input) {
        auto reg = input.vReg();
        assert(reg);
        reads().typedAdd(context, reg);
        setInputs(inputs().typedAdd(context, reg));
    }
};

class LabelLIR : public LIRBase<LabelLIR, schema::HadronLabelLIRSchema, LIR> {
public:
    LabelLIR(): LIRBase<LabelLIR, schema::HadronLabelLIRSchema, LIR>() {}
    explicit LabelLIR(schema::HadronLabelLIRSchema* instance):
            LIRBase<LabelLIR, schema::HadronLabelLIRSchema, LIR>(instance) {}
    explicit LabelLIR(Slot instance): LIRBase<LabelLIR, schema::HadronLabelLIRSchema, LIR>(instance) {}
    ~LabelLIR() {}

    LabelId labelId() const { return LabelId(m_instance->labelId); }
    void setLabelId(LabelId lbl) { m_instance->labelId = lbl.slot(); }

    TypedArray<LabelId> predecessors() const { return TypedArray<LabelId>(m_instance->predecessors); }
    void setPredecessors(TypedArray<LabelId> pred) { m_instance->predecessors = pred.slot(); }

    TypedArray<LabelId> successors() const { return TypedArray<LabelId>(m_instance->successors); }
    void setSuccessors(TypedArray<LabelId> succ) { m_instance->successors = succ.slot(); }

    TypedArray<LIR> phis() const { return TypedArray<LIR>(m_instance->phis); }
    void setPhis(TypedArray<LIR> p) { m_instance->phis = p.slot(); }

    Integer loopReturnPredIndex() const { return Integer(m_instance->loopReturnPredIndex); }
    void setLoopReturnPredIndex(Integer i) { m_instance->loopReturnPredIndex = i.slot(); }
};

class LoadConstantLIR : public LIRBase<LoadConstantLIR, schema::HadronLoadConstantLIRSchema, LIR> {
public:
    LoadConstantLIR(): LIRBase<LoadConstantLIR, schema::HadronLoadConstantLIRSchema, LIR>() {}
    explicit LoadConstantLIR(schema::HadronLoadConstantLIRSchema* instance):
            LIRBase<LoadConstantLIR, schema::HadronLoadConstantLIRSchema, LIR>(instance) {}
    explicit LoadConstantLIR(Slot instance):
            LIRBase<LoadConstantLIR, schema::HadronLoadConstantLIRSchema, LIR>(instance) {}
    ~LoadConstantLIR() {}

    Slot constant() const { return m_instance->constant; }
    void setConstant(Slot c) { m_instance->constant = c; }
};

class LoadFromPointerLIR : public LIRBase<LoadFromPointerLIR, schema::HadronLoadFromPointerLIRSchema, LIR> {
public:
    LoadFromPointerLIR(): LIRBase<LoadFromPointerLIR, schema::HadronLoadFromPointerLIRSchema, LIR>() {}
    explicit LoadFromPointerLIR(schema::HadronLoadFromPointerLIRSchema* instance):
            LIRBase<LoadFromPointerLIR, schema::HadronLoadFromPointerLIRSchema, LIR>(instance) {}
    explicit LoadFromPointerLIR(Slot instance):
            LIRBase<LoadFromPointerLIR, schema::HadronLoadFromPointerLIRSchema, LIR>(instance) {}
    ~LoadFromPointerLIR() {}

    VReg pointer() const { return VReg(m_instance->pointer); }
    void setPointer(VReg p) { m_instance->pointer = p.slot(); }

    Integer offset() const { return Integer(m_instance->offset); }
    void setOffset(Integer off) { m_instance->offset = off.slot(); }
};

class StoreToPointerLIR : public LIRBase<StoreToPointerLIR, schema::HadronStoreToPointerLIRSchema, LIR> {
public:
    StoreToPointerLIR(): LIRBase<StoreToPointerLIR, schema::HadronStoreToPointerLIRSchema, LIR>() {}
    explicit StoreToPointerLIR(schema::HadronStoreToPointerLIRSchema* instance):
            LIRBase<StoreToPointerLIR, schema::HadronStoreToPointerLIRSchema, LIR>(instance) {}
    explicit StoreToPointerLIR(Slot instance):
            LIRBase<StoreToPointerLIR, schema::HadronStoreToPointerLIRSchema, LIR>(instance) {}
    ~StoreToPointerLIR() {}
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_LIR_HPP_