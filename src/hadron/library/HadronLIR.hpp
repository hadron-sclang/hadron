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

// class PhiLIR

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
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_LIR_HPP_