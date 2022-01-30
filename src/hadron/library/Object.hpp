#ifndef SRC_HADRON_LIBRARY_OBJECT_HPP_
#define SRC_HADRON_LIBRARY_OBJECT_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Heap.hpp"
#include "hadron/Slot.hpp"
#include "hadron/ThreadContext.hpp"

#include "hadron/library/Schema.hpp"
#include "schema/Common/Core/ObjectSchema.hpp"

#include <cassert>

namespace hadron {
namespace library {

template<typename T, typename S>
class Object {
public:
    // Wraps an existing schema instance. Will assert if the type of the schema doesn't exactly match S. For wrapping
    // without type checking, use wrapUnsafe().
    explicit Object(S* instance): m_instance(instance) { assert(m_instance->_className == S::kNameHash); }
    explicit Object(Slot instance): m_instance(instance.getPointer()) {
        assert(m_instance->_className == S::kNameHash);
    }

    // Destructor must deliberately do nothing as it wraps a garbage-collected pointer.
    ~Object() {}

    static inline T alloc(ThreadContext* context) {
        S* instance = context->heap->allocateNew(sizeof(S));
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

    inline S* instance() { return m_instance; }
    inline Slot slot() { return Slot::makePointer(m_instance); }

protected:
    S* m_instance;

private:
    Object(): m_instance(nullptr) {}
};

} // namespace library
} // namespace hadron


#endif // SRC_HADRON_LIBRARY_OBJECT_HPP_