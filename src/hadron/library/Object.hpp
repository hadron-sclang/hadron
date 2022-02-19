#ifndef SRC_HADRON_LIBRARY_OBJECT_HPP_
#define SRC_HADRON_LIBRARY_OBJECT_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Heap.hpp"
#include "hadron/Slot.hpp"
#include "hadron/ThreadContext.hpp"

#include "hadron/library/Schema.hpp"
#include "hadron/schema/Common/Core/ObjectSchema.hpp"

#include <cassert>

namespace hadron {
namespace library {

// Our Object class can wrap any heap-allocated precompiled Schema struct. It uses the Curious Recurring Template
// Pattern, or CRTP, to provide static function dispatch without adding any storage overhead for a vtable. It is a
// veneer over Slot pointers that provides the strong typing we want when using sclang objects in C++ code.
template<typename T, typename S>
class Object {
public:
    Object(): m_instance(nullptr) {}

    // Wraps an existing schema instance. Will assert if the type of the schema doesn't exactly match S. For wrapping
    // without type checking, use wrapUnsafe().
    explicit Object(S* instance): m_instance(instance) {
        if (m_instance) {
            assert(m_instance->_className == S::kNameHash);
        }
    }
    explicit Object(Slot instance) {
        if (instance.isNil()) { m_instance = nullptr; }
        else {
            m_instance = reinterpret_cast<S*>(instance.getPointer());
            assert(m_instance->_className == S::kNameHash);
        }
    }

    // Destructor must deliberately do nothing as it wraps a garbage-collected pointer.
    ~Object() {}

    // Optional initialization, sets all members to nil.
    void initToNil() {
        if (!m_instance) { return; }
        Slot* s = reinterpret_cast<Slot*>(reinterpret_cast<int8_t*>(m_instance) + sizeof(Schema));
        for (size_t i = 0; i < (m_instance->_sizeInBytes - sizeof(Schema)) / sizeof(Slot); ++i) {
            s[i] = Slot::makeNil();
        }
    }

    static inline T alloc(ThreadContext* context) {
        S* instance = reinterpret_cast<S*>(context->heap->allocateNew(sizeof(S)));
        instance->_className = S::kNameHash;
        instance->_sizeInBytes = sizeof(S);
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

    /// Useful to remove const from Array accessors.
    T clone() const { return T(m_instance); }

    inline S* instance() { return m_instance; }
    inline Slot slot() { return Slot::makePointer(m_instance); }
    inline bool isNil() { return m_instance == nullptr; }

protected:
    S* m_instance;
};

} // namespace library
} // namespace hadron


#endif // SRC_HADRON_LIBRARY_OBJECT_HPP_