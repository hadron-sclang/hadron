#ifndef SRC_HADRON_LIBRARY_ARRAY_HPP_
#define SRC_HADRON_LIBRARY_ARRAY_HPP_

#include "hadron/Slot.hpp"

#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/schema/Common/Collections/ArraySchema.hpp"

namespace hadron {
namespace library {

// The element type of Array is always going to be Slot, which means that Arrays naturally support heterogenous types as
// they can store anything that fits in a Slot. For C++-side access to sclang Arrays we also provide type wrappers for
// arrays of homogeneous types that automatically wrap and unwrap Slots into the assigned type.
class Array : public ArrayedCollection<Array, schema::ArraySchema, Slot> {
public:
    Array(): ArrayedCollection<Array, schema::ArraySchema, Slot>() {}
    explicit Array(schema::ArraySchema* instance): ArrayedCollection<Array, schema::ArraySchema, Slot>(instance) {}
    explicit Array(Slot instance): ArrayedCollection<Array, schema::ArraySchema, Slot>(instance) {}
    ~Array() {}
};

template<typename T>
class TypedArray : public Array {
public:
    TypedArray(): Array() {}
    TypedArray(schema::ArraySchema* instance): Array(instance) {}
    TypedArray(Slot instance): Array(instance) {}

    static TypedArray<T> typedArrayAlloc(ThreadContext* context, int32_t maxSize) {
        Array a = arrayAlloc(context, maxSize);
        return TypedArray<T>(a.instance());
    }

    T typedAt(int32_t index) const { return T(at(index)); }
    void typedAdd(ThreadContext* context, T element) { add(context, Slot::makePointer(element.m_instance)); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_ARRAY_HPP_