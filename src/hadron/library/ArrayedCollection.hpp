#ifndef SRC_HADRON_LIBRARY_ARRAYED_COLLECTION_HPP_
#define SRC_HADRON_LIBRARY_ARRAYED_COLLECTION_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Heap.hpp"
#include "hadron/ThreadContext.hpp"

#include "hadron/library/SequenceableCollection.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/schema/Common/Collections/ArrayedCollectionSchema.hpp"

#include <cstring>

namespace hadron {
namespace library {

template<typename T, typename S, typename E>
class ArrayedCollection : public SequenceableCollection<T, S> {
public:
    ArrayedCollection(): SequenceableCollection<T, S>() {}
    explicit ArrayedCollection(S* instance): SequenceableCollection<T, S>(instance) {}
    explicit ArrayedCollection(Slot instance): SequenceableCollection<T, S>(instance) {}
    ~ArrayedCollection() {}

    static T arrayAlloc(ThreadContext* context, int32_t maxSize = 0) {
        S* instance = arrayAllocRaw(context, maxSize);
        instance->schema._className = S::kNameHash;
        instance->schema._sizeInBytes = sizeof(S);
        return T(instance);
    }

    // Produces a new T with a copy of the values of this. Can specify an optional capacity to make the new array at.
    T copy(ThreadContext* context, int32_t maxSize = 0) const {
        maxSize = std::max(maxSize, size());
        if (maxSize > 0) {
            const T& t = static_cast<const T&>(*this);
            if (t.m_instance) {
                S* instance = arrayAllocRaw(context, maxSize);
                std::memcpy(reinterpret_cast<void*>(instance), reinterpret_cast<const void*>(t.m_instance),
                        t.m_instance->schema._sizeInBytes);
                return T(instance);
            } else {
                // Copying an empty array but requesting a nonzero maxSize so we just create a new array with that size.
                return arrayAlloc(context, maxSize);
            }
        }

        return T();
    }

    // Always returns size in number of elements.
    int32_t size() const {
        const T& t = static_cast<const T&>(*this);
        if (t.m_instance == nullptr) { return 0; }
        int32_t elementsSize = (t.m_instance->schema._sizeInBytes - sizeof(S)) / sizeof(E);
        assert(elementsSize >= 0);
        return elementsSize;
    }

    E at(int32_t index) const {
        assert(0 <= index && index < size());
        return *(start() + index);
    }

    E first() const {
        return at(0);
    }

    E last() const {
        return at(size() - 1);
    }

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
            resize(context, oldSize + coll.size());
            std::memcpy(reinterpret_cast<int8_t*>(t.m_instance) + sizeof(S) + (oldSize * sizeof(E)),
                coll.start(), coll.size() * sizeof(E));
        }
        return t;
    }

    // Returns a pointer to the start of the elements, which is just past the schema.
    inline E* start() const {
        const T& t = static_cast<const T&>(*this);
        if (t.m_instance == nullptr) { return nullptr; }
        return reinterpret_cast<E*>((reinterpret_cast<uint8_t*>(t.m_instance) + sizeof(S)));
    }

    int32_t capacity(ThreadContext* context) const {
        const T& t = static_cast<const T&>(*this);
        if (t.m_instance == nullptr) { return 0; }
        auto allocSize = context->heap->getAllocationSize(t.m_instance);
        assert(allocSize > 0);
        return (allocSize - sizeof(S)) / sizeof(E);
    }

    // newSize is in number of elements. If adding elements they are uninitialized.
    void resize(ThreadContext* context, int32_t newSize) {
        T& t = static_cast<T&>(*this);
        if (newSize > capacity(context)) {
            if (t.m_instance) {
                T newArray = copy(context, newSize);
                t.m_instance = newArray.instance();
            } else {
                S* newArray = arrayAllocRaw(context, newSize);
                newArray->schema._className = S::kNameHash;
                t.m_instance = newArray;
            }
        }

        if (t.m_instance) {
            t.m_instance->schema._sizeInBytes = sizeof(S) + (newSize * sizeof(E));
        }
    }

    // Returns an int32_t with the index of item, or nil if item not found.
    Slot indexOf(E item) const {
        for (int32_t i = 0; i < size(); ++i) {
            if (item == at(i)) {
                return Slot::makeInt32(i);
            }
        }
        return Slot::makeNil();
    }

protected:
    static S* arrayAllocRaw(ThreadContext* context, int32_t numberOfElements) {
        int32_t size = sizeof(S) + (numberOfElements * sizeof(E));
        return reinterpret_cast<S*>(context->heap->allocateNew(size));
    }
};

template<typename T, typename S, typename E>
class RawArray : public ArrayedCollection<T, S, E> {
public:
    RawArray(): ArrayedCollection<T, S, E>() {}
    explicit RawArray(S* instance): ArrayedCollection<T, S, E>(instance) {}
    explicit RawArray(Slot instance): ArrayedCollection<T, S, E>(instance) {}
    ~RawArray() {}
};

class Int8Array : public RawArray<Int8Array, schema::Int8ArraySchema, int8_t> {
public:
    Int8Array(): RawArray<Int8Array, schema::Int8ArraySchema, int8_t>() {}
    explicit Int8Array(schema::Int8ArraySchema* instance):
            RawArray<Int8Array, schema::Int8ArraySchema, int8_t>(instance) {}
    explicit Int8Array(Slot instance): RawArray<Int8Array, schema::Int8ArraySchema, int8_t>(instance) {}
    ~Int8Array() {}

    static Int8Array arrayAllocJIT(ThreadContext* context, size_t byteSize, size_t& maxSize) {
        size_t size = sizeof(schema::Int8ArraySchema) + byteSize;
        schema::Int8ArraySchema* instance = reinterpret_cast<schema::Int8ArraySchema*>(
            context->heap->allocateJIT(size, maxSize));
        instance->schema._className = schema::Int8ArraySchema::kNameHash;
        instance->schema._sizeInBytes = size;
        return Int8Array(instance);
    }
};

class Int32Array : public RawArray<Int32Array, schema::Int32ArraySchema, int32_t> {
public:
    Int32Array(): RawArray<Int32Array, schema::Int32ArraySchema, int32_t>() {}
    explicit Int32Array(schema::Int32ArraySchema* instance):
            RawArray<Int32Array, schema::Int32ArraySchema, int32_t>(instance) {}
    explicit Int32Array(Slot instance): RawArray<Int32Array, schema::Int32ArraySchema, int32_t>(instance) {}
    ~Int32Array() {}
};

class SymbolArray : public RawArray<SymbolArray, schema::SymbolArraySchema, Symbol> {
public:
    SymbolArray(): RawArray<SymbolArray, schema::SymbolArraySchema, Symbol>() {}
    explicit SymbolArray(schema::SymbolArraySchema* instance):
        RawArray<SymbolArray, schema::SymbolArraySchema, Symbol>(instance) {}
    explicit SymbolArray(Slot instance):
        RawArray<SymbolArray, schema::SymbolArraySchema, Symbol>(instance) {}
    ~SymbolArray() {}
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_ARRAYED_COLLECTION_HPP_