#ifndef SRC_HADRON_LIBRARY_ARRAYED_COLLECTION_HPP_
#define SRC_HADRON_LIBRARY_ARRAYED_COLLECTION_HPP_

#include "hadron/Hash.hpp"
#include "hadron/library/Integer.hpp"
#include "hadron/library/SequenceableCollection.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/schema/Common/Collections/ArrayedCollectionSchema.hpp"
#include "hadron/ThreadContext.hpp"

#include <cstring>

namespace hadron { namespace library {

template <typename T, typename S, typename E> class ArrayedCollection : public SequenceableCollection<T, S> {
public:
    ArrayedCollection(): SequenceableCollection<T, S>() { }
    explicit ArrayedCollection(S* instance): SequenceableCollection<T, S>(instance) { }
    explicit ArrayedCollection(Slot instance): SequenceableCollection<T, S>(instance) { }
    ~ArrayedCollection() { }

    static T arrayAlloc(ThreadContext* context, int32_t maxSize = 0) {
        S* instance = arrayAllocRaw(context, maxSize);
        instance->schema.sizeInBytes = sizeof(S);
        return T(instance);
    }

    // Produces a new T with a copy of the values of this. Can specify an optional capacity to make the new array with.
    T copy(ThreadContext* context, int32_t maxSize = 0) const {
        const T& t = static_cast<const T&>(*this);
        maxSize = std::max(maxSize, t.size());
        if (maxSize > 0) {
            if (t.m_instance) {
                S* instance = arrayAllocRaw(context, maxSize);
                std::memcpy(reinterpret_cast<int8_t*>(instance) + sizeof(S),
                            reinterpret_cast<const int8_t*>(t.m_instance) + sizeof(S),
                            t.m_instance->schema.sizeInBytes - sizeof(S));
                instance->schema.sizeInBytes = t.m_instance->schema.sizeInBytes;
                return T(instance);
            } else {
                // Copying an empty array but requesting a nonzero maxSize so we just create a new array with that size.
                return arrayAlloc(context, maxSize);
            }
        }

        return T();
    }

    // Returns a new T with a copy of all elements from |start| to |end| inclusive. If |end| < |start|, returns an
    // empty list.
    T copyRange(ThreadContext* context, int32_t start, int32_t end) const {
        const T& t = static_cast<const T&>(*this);
        end = std::min(end, t.size() - 1);
        if (end < start) {
            return T::arrayAlloc(context);
        }
        auto newArray = T();
        auto newSize = end - start + 1;
        newArray.resize(context, newSize);
        std::memcpy(reinterpret_cast<void*>(newArray.start()),
                    reinterpret_cast<int8_t*>(t.start()) + (start * sizeof(E)), newSize * sizeof(E));
        return newArray;
    }

    // Always returns size in number of elements.
    int32_t size() const {
        const T& t = static_cast<const T&>(*this);
        if (t.m_instance == nullptr) {
            return 0;
        }
        int32_t elementsSize = (t.m_instance->schema.sizeInBytes - sizeof(S)) / sizeof(E);
        assert(elementsSize >= 0);
        return elementsSize;
    }

    // TODO(https://github.com/hadron-sclang/hadron/issues/109): add C++ style access semantics

    E at(int32_t index) const {
        assert(0 <= index && index < size());
        return *(start() + index);
    }

    E first() const { return at(0); }

    E last() const { return at(size() - 1); }

    void put(int32_t index, E value) {
        assert(index < size());
        *(start() + index) = value;
    }

    T& add(ThreadContext* context, E element) {
        int32_t oldSize = size();
        resize(context, oldSize + 1);
        *(start() + oldSize) = element;
        return static_cast<T&>(*this);
    }

    T& addAll(ThreadContext* context, const ArrayedCollection<T, S, E>& coll) {
        T& t = static_cast<T&>(*this);
        if (coll.size()) {
            int32_t oldSize = size();
            t.resize(context, oldSize + coll.size());
            std::memcpy(reinterpret_cast<int8_t*>(t.m_instance) + sizeof(S) + (oldSize * sizeof(E)), coll.start(),
                        coll.size() * sizeof(E));
        }
        return t;
    }

    T& insert(ThreadContext* context, int32_t index, E item) {
        T& t = static_cast<T&>(*this);
        if (index == t.size()) {
            return add(context, item);
        }

        assert(0 <= index && index < t.size());

        // If we need to create a new array for resizing, move the elements while copying them, thus avoiding
        // the redundant copy of the unshifted elements in resize().
        if (t.capacity() == t.size()) {
            S* newArray = T::arrayAllocRaw(context, t.size() + 1);
            newArray->schema.sizeInBytes = sizeof(S) + ((t.size() + 1) * sizeof(E));
            // Copy elements before index into the new array.
            std::memcpy(reinterpret_cast<int8_t*>(newArray) + sizeof(S), t.start(), index * sizeof(E));
            // Copy remaining elements into place shifted one right.
            std::memcpy(reinterpret_cast<int8_t*>(newArray) + sizeof(S) + ((index + 1) * sizeof(E)),
                        reinterpret_cast<int8_t*>(t.start()) + (index * sizeof(E)), sizeof(E) * (t.size() - index));
            t.m_instance = newArray;
            t.put(index, item);
            return t;
        }

        t.resize(context, t.size() + 1);
        // Shift elements starting at index to the right by one.
        std::memmove(reinterpret_cast<int8_t*>(t.start()) + ((index + 1) * sizeof(E)),
                     reinterpret_cast<int8_t*>(t.start()) + (index * sizeof(E)), sizeof(E) * (t.size() - index));
        t.put(index, item);

        return t;
    }

    void removeAt(ThreadContext* context, int32_t index) {
        T& t = static_cast<T&>(*this);
        assert(0 <= index && index < t.size());

        // Shift elements starting at index + 1 to the left by one.
        if (index < t.size() - 1) {
            std::memmove(reinterpret_cast<int8_t*>(t.start()) + (index * sizeof(E)),
                         reinterpret_cast<int8_t*>(t.start()) + ((index + 1) * sizeof(E)),
                         (t.size() - index - 1) * sizeof(E));
        }

        t.resize(context, t.size() - 1);
    }

    // Returns a pointer to the start of the elements, which is just past the schema.
    inline E* start() const {
        const T& t = static_cast<const T&>(*this);
        if (t.m_instance == nullptr) {
            return nullptr;
        }
        return reinterpret_cast<E*>((reinterpret_cast<uint8_t*>(t.m_instance) + sizeof(S)));
    }

    int32_t capacity() const {
        const T& t = static_cast<const T&>(*this);
        if (t.m_instance == nullptr) {
            return 0;
        }
        auto allocSize = t.m_instance->schema.allocationSize;
        assert(allocSize > 0);
        return (allocSize - sizeof(S)) / sizeof(E);
    }

    // newSize is in number of elements. If adding elements they are uninitialized.
    void resize(ThreadContext* context, int32_t newSize) {
        T& t = static_cast<T&>(*this);
        if (newSize > capacity()) {
            if (t.m_instance) {
                T newArray = copy(context, newSize);
                t.m_instance = newArray.instance();
            } else {
                S* newArray = arrayAllocRaw(context, newSize);
                t.m_instance = newArray;
            }
        }

        if (t.m_instance) {
            t.m_instance->schema.sizeInBytes = sizeof(S) + (newSize * sizeof(E));
        }
    }

    // Returns an int32_t with the index of item, or nil if item not found.
    Integer indexOf(E item) const {
        for (int32_t i = 0; i < size(); ++i) {
            if (item == at(i)) {
                return Integer(i);
            }
        }
        return Integer();
    }

protected:
    static S* arrayAllocRaw(ThreadContext* context, int32_t numberOfElements) {
        int32_t size = sizeof(S) + (numberOfElements * sizeof(E));
        int32_t allocationSize = 0;
        S* array = reinterpret_cast<S*>(context->heap->allocateNew(size, allocationSize));
        array->schema.className = S::kNameHash;
        array->schema.allocationSize = allocationSize;
        return array;
    }
};

template <typename T, typename S, typename E> class RawArray : public ArrayedCollection<T, S, E> {
public:
    RawArray(): ArrayedCollection<T, S, E>() { }
    explicit RawArray(S* instance): ArrayedCollection<T, S, E>(instance) { }
    explicit RawArray(Slot instance): ArrayedCollection<T, S, E>(instance) { }
    ~RawArray() { }
};

class Int8Array : public RawArray<Int8Array, schema::Int8ArraySchema, int8_t> {
public:
    Int8Array(): RawArray<Int8Array, schema::Int8ArraySchema, int8_t>() { }
    explicit Int8Array(schema::Int8ArraySchema* instance):
        RawArray<Int8Array, schema::Int8ArraySchema, int8_t>(instance) { }
    explicit Int8Array(Slot instance): RawArray<Int8Array, schema::Int8ArraySchema, int8_t>(instance) { }
    ~Int8Array() { }
};

class Int32Array : public RawArray<Int32Array, schema::Int32ArraySchema, int32_t> {
public:
    Int32Array(): RawArray<Int32Array, schema::Int32ArraySchema, int32_t>() { }
    explicit Int32Array(schema::Int32ArraySchema* instance):
        RawArray<Int32Array, schema::Int32ArraySchema, int32_t>(instance) { }
    explicit Int32Array(Slot instance): RawArray<Int32Array, schema::Int32ArraySchema, int32_t>(instance) { }
    ~Int32Array() { }
};

class SymbolArray : public RawArray<SymbolArray, schema::SymbolArraySchema, Symbol> {
public:
    SymbolArray(): RawArray<SymbolArray, schema::SymbolArraySchema, Symbol>() { }
    explicit SymbolArray(schema::SymbolArraySchema* instance):
        RawArray<SymbolArray, schema::SymbolArraySchema, Symbol>(instance) { }
    explicit SymbolArray(Slot instance): RawArray<SymbolArray, schema::SymbolArraySchema, Symbol>(instance) { }
    ~SymbolArray() { }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_ARRAYED_COLLECTION_HPP_