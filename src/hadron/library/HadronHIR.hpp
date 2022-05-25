#ifndef SRC_HADRON_LIBRARY_HADRON_HIR_HPP_
#define SRC_HADRON_LIBRARY_HADRON_HIR_HPP_

#include "hadron/library/Object.hpp"
#include "hadron/schema/HLang/HadronHIRSchema.hpp"

namespace hadron {
namespace library {

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

    int32_t id() const {
        const T& t = static_cast<const T&>(*this);
        return t.m_instance->id.getInt32();
    }
    void setId(int32_t i) {
        T& t = static_cast<T&>(*this);
        t.m_instance->id = Slot::makeInt32(i);
    }

    TypeFlags typeFlags() const {
        const T& t = static_cast<const T&>(*this);
        return static_cast<TypeFlags>(t.m_instance->typeFlags.getInt32());
    }
    void setTypeFlags(TypeFlags f) {
        T& t = static_cast<T&>(*this);
        t.m_instance->typeFlags = Slot::makeInt32(f);
    }
};

class HIR : public HIRBase<HIR, schema::HadronHIRSchema, HIR> {
public:
    HIR(): HIRBase<HIR, schema::HadronHIRSchema, HIR>() {}
    explicit HIR(schema::HadronHIRSchema* instance): HIRBase<HIR, schema::HadronHIRSchema, HIR>(instance) {}
    explicit HIR(Slot instance): HIRBase<HIR, schema::HadronHIRSchema, HIR>(instance) {}
    ~HIR() {}
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_HIR_HPP_