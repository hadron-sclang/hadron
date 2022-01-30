#ifndef SRC_HADRON_LIBRARY_ARRAY_HPP_
#define SRC_HADRON_LIBRARY_ARRAY_HPP_

#include "hadron/Slot.hpp"

#include "hadron/library/ArrayedCollection.hpp"
#include "schema/Common/Collections/ArraySchema.hpp"

namespace hadron {
namespace library {

// The element type of Array is always going to be Slot, which means that Arrays naturally support heterogenous types as
// they can store anything that fits in a Slot. For C++-side access to sclang Arrays we also provide type wrappers for
// arrays of homogeneous types that automatically wrap and unwrap Slots into the assigned type.
class Array : public ArrayedCollection<Array, schema::ArraySchema, Slot> {
public:
    Array() = delete;
    
};

template<typename T>
TypedArray : public Array {
public:
    T atTyped(int32_t index) const { return T(at(index)); }
    void addTyped(T element) { add(Slot::makePointer(element.m_instance)); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_ARRAY_HPP_