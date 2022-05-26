#ifndef SRC_HADRON_LIBRARY_HADRON_HIR_HPP_
#define SRC_HADRON_LIBRARY_HADRON_HIR_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/HadronCFG.hpp"
#include "hadron/library/Integer.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/Set.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/schema/HLang/HadronHIRSchema.hpp"

namespace hadron {
namespace library {

// HIR uses plain Integers as unique identifiers for values. We use the HIRId alias to help clarify when we are
// referring to HIRId values instead of some other Integer identifier.
using HIRId = Integer;

template<typename T, typename S, typename B>
class HIRBase : public Object<T, S> {
public:
    HIRBase(): Object<T, S>() {}
    explicit HIRBase(S* instance): Object<T, S>(instance) {}
    explicit HIRBase(Slot instance): Object<T, S>(instance) {}
    ~HIRBase() {}

    B toBase() const {
        const T& t = static_cast<const T&>(*this);
        return B::wrapUnsafe(Slot::makePointer(reinterpret_cast<library::Schema*>(t.m_instance)));
    }

    HIRId id() const {
        const T& t = static_cast<const T&>(*this);
        return HIRId(t.m_instance->id);
    }
    void setId(HIRId i) {
        T& t = static_cast<T&>(*this);
        t.m_instance->id = i.slot();
    }

    TypeFlags typeFlags() const {
        const T& t = static_cast<const T&>(*this);
        return static_cast<TypeFlags>(t.m_instance->typeFlags.getInt32());
    }
    void setTypeFlags(TypeFlags f) {
        T& t = static_cast<T&>(*this);
        t.m_instance->typeFlags = Slot::makeInt32(f);
    }

    IdentitySet reads() const {
        const T& t = static_cast<const T&>(*this);
        return IdentitySet(t.m_instance->reads);
    }
    void setReads(IdentitySet r) {
        T& t = static_cast<T&>(*this);
        t.m_instance->reads = r.slot();
    }

    IdentitySet consumers() const {
        const T& t = static_cast<const T&>(*this);
        return IdentitySet(t.m_instance->consumers);
    }
    void setConsumers(IdentitySet c) {
        T& t = static_cast<T&>(*this);
        t.m_instance->consumers = c.slot();
    }

};

class HIR : public HIRBase<HIR, schema::HadronHIRSchema, HIR> {
public:
    HIR(): HIRBase<HIR, schema::HadronHIRSchema, HIR>() {}
    explicit HIR(schema::HadronHIRSchema* instance): HIRBase<HIR, schema::HadronHIRSchema, HIR>(instance) {}
    explicit HIR(Slot instance): HIRBase<HIR, schema::HadronHIRSchema, HIR>(instance) {}
    ~HIR() {}
};

class BlockLiteralHIR : public HIRBase<BlockLiteralHIR, schema::HadronBlockLiteralHIRSchema, HIR> {
public:
    BlockLiteralHIR(): HIRBase<BlockLiteralHIR, schema::HadronBlockLiteralHIRSchema, HIR>() {}
    explicit BlockLiteralHIR(schema::HadronBlockLiteralHIRSchema* instance):
            HIRBase<BlockLiteralHIR, schema::HadronBlockLiteralHIRSchema, HIR>(instance) {}
    explicit BlockLiteralHIR(Slot instance):
            HIRBase<BlockLiteralHIR, schema::HadronBlockLiteralHIRSchema, HIR>(instance) {}
    ~BlockLiteralHIR() {}

    CFGFrame frame() const { return CFGFrame(m_instance->frame); }
    void setFrame(CFGFrame f) { m_instance->frame = f.slot(); }

    FunctionDef functionDef() const { return FunctionDef(m_instance->functionDef); }
    void setFunctionDef(FunctionDef f) { m_instance->functionDef = f.slot(); }
};

class PhiHIR : public HIRBase<PhiHIR, schema::HadronPhiHIRSchema, HIR> {
public:
    PhiHIR(): HIRBase<PhiHIR, schema::HadronPhiHIRSchema, HIR>() {}
    explicit PhiHIR(schema::HadronPhiHIRSchema* instance): HIRBase<PhiHIR, schema::HadronPhiHIRSchema, HIR>(instance) {}
    explicit PhiHIR(Slot instance): HIRBase<PhiHIR, schema::HadronPhiHIRSchema, HIR>(instance) {}
    ~PhiHIR() {}

    Symbol name(ThreadContext* context) const { return Symbol(context, m_instance->name); }
    void setName(Symbol n) { m_instance->name = n.slot(); }

    Int32Array inputs() const { return Int32Array(m_instance->inputs); }
    void setInputs(Int32Array a) { m_instance->inputs = a.slot(); }

};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_HIR_HPP_