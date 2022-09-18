#ifndef SRC_HADRON_LIBRARY_ARRAY_HPP_
#define SRC_HADRON_LIBRARY_ARRAY_HPP_

#include "hadron/Slot.hpp"

#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/schema/Common/Collections/ArraySchema.hpp"

namespace hadron { namespace library {

// TODO(https://github.com/hadron-sclang/hadron/issues/111): Templatize all the data structures!

// The element type of Array is always going to be Slot, which means that Arrays naturally support heterogenous types as
// they can store anything that fits in a Slot. For C++-side access to sclang Arrays we also provide type wrappers for
// arrays of homogeneous types that automatically wrap and unwrap Slots into the assigned type.
class Array : public ArrayedCollection<Array, schema::ArraySchema, Slot> {
public:
    Array(): ArrayedCollection<Array, schema::ArraySchema, Slot>() { }
    explicit Array(schema::ArraySchema* instance): ArrayedCollection<Array, schema::ArraySchema, Slot>(instance) { }
    explicit Array(Slot instance): ArrayedCollection<Array, schema::ArraySchema, Slot>(instance) { }
    ~Array() { }

    // Makes a new array of size |indexedSize| with each element set to nil.
    static Array newClear(ThreadContext* context, int32_t indexedSize) {
        Array array = Array::alloc(context, indexedSize);
        for (int32_t i = 0; i < indexedSize; ++i) {
            array.put(i, Slot::makeNil());
        }
        return array;
    }

    // Supports IdentitySet, searches the array for an element with identityHash matching |key|, or the index of the
    // empty element if no matching element found.
    int32_t atIdentityHash(Slot key) const {
        auto hash = key.identityHash();
        auto index = (hash % size());
        auto element = at(index);
        while (element && element.identityHash() != hash) {
            index = (index + 1) % size();
            element = at(index);
        }

        return index;
    }

    // Supports IdentityDictionary, searches the array assuming the elements are in key/value pairs. Returns the index
    // of the element with identityHash matching |key|, or the index of the empty element if no matching element found.
    int32_t atIdentityHashInPairs(Slot key) const {
        auto hash = key.identityHash();
        // Keys are always at even indexes followed by their value pair at odd, so mask off the least significant bit to
        // compute even index.
        auto index = (hash % size()) & (~1);
        auto element = at(index);
        while (element && element.identityHash() != hash) {
            index = (index + 2) % size();
            element = at(index);
        }

        return index;
    }

    // Returns a new array with the members of this array in reverse order.
    Array reverse(ThreadContext* context) const {
        auto r = Array::arrayAlloc(context, size());
        for (int32_t i = size() - 1; i >= 0; --i) {
            r = r.add(context, at(i));
        }
        return r;
    }
};

template <typename T> class TypedArray : public Array {
public:
    TypedArray(): Array() { }
    explicit TypedArray(schema::ArraySchema* instance): Array(instance) { }
    explicit TypedArray(Slot instance): Array(instance) { }
    ~TypedArray() { }

    // Does NOT do what it says on the tin.
    static inline TypedArray<T> wrapUnsafe(Slot instance) { return TypedArray<T>(instance); }

    static TypedArray<T> typedArrayAlloc(ThreadContext* context, int32_t maxSize = 0) {
        Array a = arrayAlloc(context, maxSize);
        return TypedArray<T>(a.instance());
    }

    static TypedArray<T> typedNewClear(ThreadContext* context, int32_t indexedSize) {
        Array a = newClear(context, indexedSize);
        return TypedArray<T>(a.instance());
    }

    TypedArray<T> typedCopyRange(ThreadContext* context, int32_t start, int32_t end) const {
        Array a = copyRange(context, start, end);
        return TypedArray<T>(a.instance());
    }

    T typedAt(int32_t index) const { return T::wrapUnsafe(at(index).slot()); }
    T typedFirst() const { return T::wrapUnsafe(first()); }
    T typedLast() const { return T::wrapUnsafe(last()); }
    void typedPut(int32_t index, T element) { put(index, element.slot()); }

    TypedArray<T>& typedAdd(ThreadContext* context, T element) {
        add(context, element.slot());
        return *this;
    }

    Slot typedIndexOf(T item) const { return indexOf(item.slot()); }

    TypedArray<T> typedReverse(ThreadContext* context) {
        auto r = reverse(context);
        return TypedArray<T>(r.instance());
    }

    TypedArray<T> typedInsert(ThreadContext* context, int32_t index, T element) {
        auto i = insert(context, index, element.slot());
        return TypedArray<T>(i.instance());
    }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_ARRAY_HPP_