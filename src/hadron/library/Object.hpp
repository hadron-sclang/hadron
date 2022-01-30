#ifndef SRC_HADRON_LIBRARY_OBJECT_HPP_
#define SRC_HADRON_LIBRARY_OBJECT_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Heap.hpp"
#include "hadron/Slot.hpp"
#include "hadron/ThreadContext.hpp"

#include "hadron/library/Schema.hpp"
#include "schema/Common/Core/ObjectSchema.hpp"

namespace hadron {
namespace library {

template<typename T, typename S>
class Object {
public:
    Object() = delete;

    // Wraps an existing schema instance.
    explicit Object(S* instance): m_instance(instance) {}

    // Destructor must deliberately do nothing as it wraps a garbage-collected pointer.
    ~Object() {}

    static inline T alloc(ThreadContext* context) {
        S* instance = context->heap->allocateNew(sizeof(S));
        instance->_className = S::kNameHash;
        instance->_sizeInBytes = sizeof(S);
        return T(instance);
    }

protected:
    S* m_instance;
};

} // namespace library
} // namespace hadron


#endif // SRC_HADRON_LIBRARY_OBJECT_HPP_