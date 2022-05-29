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
        if (t.m_instance == nullptr) { return Array(); }
        return Array(t.m_instance->array);
    }

    int32_t size() const {
        const T& t = static_cast<const T&>(*this);
        if (t.m_instance == nullptr) { return 0; }
        return t.m_instance->size.getInt32();
    }

protected:
    void setArray(Array a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->array = a.slot();
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

    // add() is not implemented as a primitive in the SuperCollider library code. This C++ implementation mimic the
    // implementation in Set. If making substantive changes to behavior in either implementation the other must change /
    // to reflect the new behavior. TODO: consider a new implementation of HadronSet, HadronIdentitySet,
    // HadronIdentityDictionary.
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

    bool contains(Slot item) const {
        auto index = array().atIdentityHash(item);
        return !array().at(index).isNil();
    }

    // Returns the item next to |i| in the unordered array, or nil if that was the last item. If |i| is nil, returns
    // the *first* item in the array, or nil in an empty IdentitySet.
    Slot next(Slot i) const {
        int32_t index = i ? array().atIdentityHash(i) + 1 : 0;
        auto element = Slot::makeNil();

        while (!element && index < array().size()) {
            element = array().at(index);
            ++index;
        }

        return element;
    }
};

template<typename V>
class TypedIdentSet : public IdentitySet {
public:
    TypedIdentSet(): IdentitySet() {}
    explicit TypedIdentSet(schema::IdentitySetSchema* instance): IdentitySet(instance) {}
    explicit TypedIdentSet(Slot instance): IdentitySet(instance) {}
    ~TypedIdentSet() {}

    static TypedIdentSet<V> makeTypedIdentSet(ThreadContext* context, int32_t capacity = 4) {
        return TypedIdentSet<V>(IdentitySet::makeIdentitySet(context, capacity).slot());
    }

    TypedArray<V> typedArray() const { return TypedArray<V>(array().slot()); }

    void typedAdd(ThreadContext* context, V item) {
        add(context, item.slot());
    }

    bool typedContains(V item) const {
        return contains(item.slot());
    }

    V typedNext(V i) const {
        return V::wrapUnsafe(next(i.slot()));
    }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_SET_HPP_