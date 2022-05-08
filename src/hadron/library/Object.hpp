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

namespace hadron {
namespace library {

// Our Object class can wrap any heap-allocated precompiled Schema struct. It uses the Curious Recurring Template
// Pattern, or CRTP, to provide static function dispatch without adding any storage overhead for a vtable. It is a
// veneer over Slot pointers that provides type checking when using sclang objects in C++ code.
template<typename T, typename S>
class Object {
public:
    Object(): m_instance(nullptr) {}
    Object(const Object& o): m_instance(o.m_instance) {}

    // Wraps an existing schema instance. Will assert if the type of the schema doesn't exactly match S. For wrapping
    // without type checking, use wrapUnsafe().
    explicit Object(S* instance): m_instance(instance) {
        if (m_instance) {
            assert(m_instance->schema._className == S::kNameHash);
        }
    }
    explicit Object(Slot instance) {
        if (instance.isNil()) { m_instance = nullptr; }
        else {
            m_instance = reinterpret_cast<S*>(instance.getPointer());
            assert(m_instance->schema._className == S::kNameHash);
        }
    }

    // Destructor must deliberately do nothing as it wraps an automatically garbage-collected pointer.
    ~Object() {}

    // Optional initialization, sets all members to nil.
    void initToNil() {
        if (!m_instance) { assert(false); return; }
        Slot* s = reinterpret_cast<Slot*>(reinterpret_cast<int8_t*>(m_instance) + sizeof(Schema));
        for (size_t i = 0; i < (m_instance->schema._sizeInBytes - sizeof(Schema)) / kSlotSize; ++i) {
            s[i] = Slot::makeNil();
        }
    }

    static inline T alloc(ThreadContext* context, int32_t extraSlots = 0) {
        size_t sizeInBytes = sizeof(S) + (extraSlots * kSlotSize);
        S* instance = reinterpret_cast<S*>(context->heap->allocateNew(sizeInBytes));
        instance->schema._className = S::kNameHash;
        instance->schema._sizeInBytes = sizeInBytes;
        return T(instance);
    }

    static inline T wrapUnsafe(Schema* schema) {
        T wrapper;
        wrapper.m_instance = reinterpret_cast<S*>(schema);
        return wrapper;
    }
    static inline T wrapUnsafe(Slot schema) {
        T wrapper;
        wrapper.m_instance = reinterpret_cast<S*>(schema.getPointer());
        return wrapper;
    }

    inline S* instance() { return m_instance; }
    inline Slot slot() const { return Slot::makePointer(reinterpret_cast<library::Schema*>(m_instance)); }
    inline bool isNil() const { return m_instance == nullptr; }
    inline Hash className() const {
        if (isNil()) { return schema::NilSchema::kNameHash; }
        return m_instance->schema._className;
    }
    explicit inline operator bool() const { return m_instance != nullptr; }
    static constexpr Hash nameHash() { return S::kNameHash; }
    static constexpr int32_t schemaSize() { return (sizeof(S) - sizeof(Schema)) / kSlotSize; }

protected:
    S* m_instance;
};

} // namespace library
} // namespace hadron


#endif // SRC_HADRON_LIBRARY_OBJECT_HPP_