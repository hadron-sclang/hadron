#ifndef SRC_HADRON_LIBRARY_ARRAYED_COLLECTION_HPP_
#define SRC_HADRON_LIBRARY_ARRAYED_COLLECTION_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Heap.hpp"
#include "hadron/ThreadContext.hpp"

#include "hadron/library/SequenceableCollection.hpp"
#include "hadron/schema/Common/Collections/ArrayedCollectionSchema.hpp"

namespace hadron {
namespace library {

template<typename T, typename S, typename E>
class ArrayedCollection : public SequenceableCollection<T, S> {
public:
    ArrayedCollection(): SequenceableCollection<T, S>() {}
    explicit ArrayedCollection(S* instance): SequenceableCollection<T, S>(instance) {}
    ~ArrayedCollection() {}

    static T arrayAlloc(ThreadContext* context, int32_t numberOfElements) {
        S* instance = arrayAllocRaw(context, numberOfElements);
        instance->_className = S::kNameHash;
        instance->_sizeInBytes = sizeof(S);
        return T(instance);
    }

    // Always returns size in number of elements.
    int32_t size() const {
        T& t = static_cast<T&>(*this);
        return (t.m_instance->_sizeInBytes - sizeof(S)) / sizeof(E);
    }

    E at(int32_t index) const { return *(start() + index); }

    void add(ThreadContext* context, E element) {
        T& t = static_cast<T&>(*this);
        resize(context, size() + 1);
        *(start() + size()) = element;
    }

    // Returns a pointer to the start of the elements, which is just past the schema.
    inline E* start() const {
        T& t = static_cast<T&>(*this);
        return reinterpret_cast<E*>((reinterpret_cast<uint8_t*>(t.m_instance) + sizeof(S)));
    }

    int32_t capacity(ThreadContext* context) const {
        T& t = static_cast<T&>(*this);
        auto allocSize = context->heap->getAllocationSize(t.m_instance);
        return (allocSize - sizeof(S)) / sizeof(E);
    }

    // newSize is in number of elements. If adding elements they are uninitialized.
    void resize(ThreadContext* context, int32_t newSize) {
        T& t = static_cast<T&>(*this);
        if (newSize > capacity()) {
            S* newArray = arrayAllocSchema(context, size() + 1);
            std::memcpy(newArray, t.m_instance, t.m_instance->sizeInBytes);
            t.m_instance = newArray;
        }
        t.m_instance->_sizeInBytes = sizeof(S) + (newSize * sizeof(E));
    }

protected:
    static S* arrayAllocRaw(ThreadContext* context, int32_t numberOfElements) {
        int32_t size = sizeof(S) + (numberOfElements * sizeof(E));
        return context->heap->allocateNew(size);
    }
};

template<typename T, typename S, typename E>
class RawArray : public ArrayedCollection<T, S, E> {
public:
    RawArray(): ArrayedCollection<T, S, E>() {}
    explicit RawArray(S* instance): ArrayedCollection<T, S, E>(instance) {}
    ~RawArray() {}
};

class Int8Array : public RawArray<Int8Array, schema::Int8ArraySchema, int8_t> {
public:
    Int8Array(): RawArray<Int8Array, schema::Int8ArraySchema, int8_t>() {}
    explicit Int8Array(schema::Int8ArraySchema* instance):
            RawArray<Int8Array, schema::Int8ArraySchema, int8_t>(instance) {}
    ~Int8Array() {}

    static Int8Array arrayAllocJIT(ThreadContext* context, size_t byteSize, size_t& maxSize) {
        size_t size = sizeof(schema::Int8ArraySchema) + byteSize;
        schema::Int8ArraySchema* instance = reinterpret_cast<schema::Int8ArraySchema*>(
            context->heap->allocateJIT(size, maxSize));
        instance->_className = schema::Int8ArraySchema::kNameHash;
        instance->_sizeInBytes = size;
        return Int8Array(instance);
    }
};

class SymbolArray : public RawArray<SymbolArray, schema::SymbolArraySchema, Hash> {
public:
    SymbolArray(): RawArray<SymbolArray, schema::SymbolArraySchema, Hash>() {}
    explicit SymbolArray(schema::SymbolArraySchema* instance):
        RawArray<SymbolArray, schema::SymbolArraySchema, Hash>(instance) {}
    ~SymbolArray() {}
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_ARRAYED_COLLECTION_HPP_