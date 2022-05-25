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

class IdentitySet : public Set<IdentitySet, schema::IdentitySetSchema> {
public:
    IdentitySet(): Set<IdentitySet, schema::IdentitySetSchema>() {}
    explicit IdentitySet(schema::IdentitySetSchema* instance): Set<IdentitySet, schema::IdentitySetSchema>(instance) {}
    explicit IdentitySet(Slot instance): Set<IdentitySet, schema::IdentitySetSchema>(instance) {}
    ~IdentitySet() {}

    static IdentitySet makeIdentitySet(ThreadContext* context, int32_t capacity = 4) {
        auto set = IdentitySet::alloc(context);
        set.setArray(Array::newClear(context, (capacity * 3) / 2));
        set.setSize(0);
        return set;
    }

    // add(), remove(), and scanFor() are not implemented as primitives in the SuperCollider library code. These C++
    // implementations mimic the implementation in Set and IdentitySet. If making substantive changes to behavior in
    // either implementation the other must change to reflect the new behavior. TODO: consider a new implementation of
    // HadronSet, HadronIdentitySet, HadronIdentityDictionary.
    void add(ThreadContext* context, Slot item) {
        assert(item);

        if ((size() * 3) / 2 >= array().size()) {
            auto newSet = IdentitySet::makeIdentitySet(context, size() * 2);
            for (int32_t i = 0; i < array().size(); ++i) {
                auto element = array().at(i);
                if (element) {
                    newSet.add(context, element);
                }
            }

            setArray(newSet.array());
        }

        auto index = array().atIdentityHash(item);
        auto existingElement = array().at(index);
        if (existingElement) {
            assert(existingElement.identityHash() == item.identityHash());
        } else {
            setSize(size() + 1);
        }
        array().put(index, item);
    }

    int scanFor(Slot item) const {
        return array().atIdentityHash(item);
    }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_SET_HPP_