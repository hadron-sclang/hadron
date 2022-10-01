#ifndef SRC_HADRON_LIBRARY_OBJECT_HPP_
#define SRC_HADRON_LIBRARY_OBJECT_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Heap.hpp"
#include "hadron/library/Schema.hpp"
#include "hadron/schema/Common/Core/NilSchema.hpp"
#include "hadron/schema/Common/Core/ObjectSchema.hpp"
#include "hadron/Slot.hpp"
#include "hadron/ThreadContext.hpp"

#include <cassert>
#include <type_traits>

namespace hadron { namespace library {

// Our Object class can wrap any heap-allocated precompiled Schema struct. It uses the Curious Recurring Template
// Pattern, or CRTP, to provide static function dispatch without adding any storage overhead for a vtable. It is a
// veneer over Slot pointers that provides type checking when using sclang objects in C++ code.
template <typename T, typename S> class Object {
public:
    Object(): m_instance(nullptr) { }
    Object(const Object& o): m_instance(o.m_instance) { }

    // Wraps an existing schema instance. Will assert if the type of the schema doesn't exactly match S. For wrapping
    // without type checking, use wrapUnsafe().
    explicit Object(S* instance): m_instance(instance) {
        if (m_instance) {
            assert(m_instance->schema.className == S::kNameHash);
        }
    }
    explicit Object(Slot instance) {
        if (instance.isNil()) {
            m_instance = nullptr;
        } else {
            m_instance = reinterpret_cast<S*>(instance.getPointer());
            assert(m_instance->schema.className == S::kNameHash);
        }
    }

    // Destructor must deliberately do nothing as it wraps an automatically garbage-collected pointer.
    ~Object() { }

    // Optional initialization, sets all members to nil.
    void initToNil() {
        if (!m_instance) {
            assert(false);
            return;
        }
        // TODO: asArray()/clear() using std::fill?
        Slot* s = reinterpret_cast<Slot*>(reinterpret_cast<int8_t*>(m_instance) + sizeof(Schema));
        for (int32_t i = 0; i < (m_instance->schema.sizeInBytes - static_cast<int32_t>(sizeof(Schema))) / kSlotSize;
             ++i) {
            s[i] = Slot::makeNil();
        }
    }

    static inline T alloc(ThreadContext* context, int32_t extraSlots = 0) {
        int32_t sizeInBytes = sizeof(S) + (extraSlots * kSlotSize);
        int32_t allocationSize = 0;
        S* instance = reinterpret_cast<S*>(context->heap->allocateNew(sizeInBytes, allocationSize));
        instance->schema.className = S::kNameHash;
        instance->schema.sizeInBytes = sizeInBytes;
        instance->schema.allocationSize = allocationSize;
        return T(instance);
    }

    static inline T wrapUnsafe(Schema* schema) {
        T wrapper;
        wrapper.m_instance = reinterpret_cast<S*>(schema);
        return wrapper;
    }
    static inline T wrapUnsafe(Slot schema) {
        T wrapper;
        if (schema.isNil()) {
            wrapper.m_instance = nullptr;
        } else {
            wrapper.m_instance = reinterpret_cast<S*>(schema.getPointer());
        }
        return wrapper;
    }

    inline S* instance() { return m_instance; }
    inline Slot slot() const { return Slot::makePointer(reinterpret_cast<library::Schema*>(m_instance)); }
    inline bool isNil() const { return m_instance == nullptr; }
    inline Hash className() const {
        if (isNil()) {
            return schema::NilSchema::kNameHash;
        }
        return m_instance->schema.className;
    }
    explicit inline operator bool() const { return m_instance != nullptr; }
    static constexpr Hash nameHash() { return S::kNameHash; }
    static constexpr int32_t schemaSize() { return (sizeof(S) - sizeof(Schema)) / kSlotSize; }

protected:
    S* m_instance;
};

class ObjectBase : public Object<ObjectBase, schema::ObjectSchema> {
public:
    ObjectBase(): Object<ObjectBase, schema::ObjectSchema>() { }
    explicit ObjectBase(schema::ObjectSchema* instance): Object<ObjectBase, schema::ObjectSchema>(instance) { }
    explicit ObjectBase(Slot instance): Object<ObjectBase, schema::ObjectSchema>(instance) { }
    ~ObjectBase() { }

    Slot _BasicNew(ThreadContext* context, int32_t maxSize);
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_OBJECT_HPP_