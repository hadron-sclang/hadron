#ifndef SRC_HADRON_LIBRARY_SET_HPP_
#define SRC_HADRON_LIBRARY_SET_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/Collection.hpp"
#include "hadron/schema/Common/Collections/SetSchema.hpp"

namespace hadron {
namespace library {

template<typename T, typename S>
class Set : public Collection<T, S> {
public:
    Set(): Collection<T, S>() {}
    explicit Set(S* instance): Collection<T, S>(instance) {}
    explicit Set(Slot instance): Collection<T, S>(instance) {}
    ~Set() {}

    Array array() const {
        const T& t = static_cast<const T&>(*this);
        return Array(t.m_instance->array);
    }
    void setArray(Array a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->array = a.slot();
    }

    int32_t size() const {
        const T& t = static_cast<const T&>(*this);
        return t.m_instance->size.getInt32();
    }
    void setSize(int32_t s) {
        T& t = static_cast<T&>(*this);
        t.m_instance->size = Slot::makeInt32(s);
    }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_SET_HPP_