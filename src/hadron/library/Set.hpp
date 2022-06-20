#ifndef SRC_HADRON_LIBRARY_SET_HPP_
#define SRC_HADRON_LIBRARY_SET_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/Collection.hpp"
#include "hadron/library/Integer.hpp"
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

template<typename T, typename S>
class IdentitySetBase : public Set<T, S> {
public:
    IdentitySetBase(): Set<T, S>() {}
    explicit IdentitySetBase(S* instance): Set<T, S>(instance) {}
    explicit IdentitySetBase(Slot instance): Set<T, S>(instance) {}
    ~IdentitySetBase() {}

    // add() is not implemented as a primitive in the SuperCollider library code. This C++ implementation mimic the
    // implementation in Set. If making substantive changes to behavior in either implementation the other must change
    // to reflect the new behavior. TODO: consider a new implementation of HadronSet, HadronIdentitySet,
    // HadronIdentityDictionary.
    void add(ThreadContext* context, Slot item) {
        T& t = static_cast<T&>(*this);
        assert(item);

        if ((t.size() * 3) / 2 >= t.array().size()) {
            auto newSet = T::makeIdentitySet(context, t.size() * 2);
            for (int32_t i = 0; i < t.array().size(); ++i) {
                auto element = t.array().at(i);
                if (element) {
                    newSet.add(context, element);
                }
            }

            t.setArray(newSet.array());
        }

        auto index = t.array().atIdentityHash(item);
        auto existingElement = t.array().at(index);
        if (existingElement) {
            assert(existingElement.identityHash() == item.identityHash());
        } else {
            t.setSize(t.size() + 1);
        }
        t.array().put(index, item);
    }

    bool contains(Slot item) const {
        const T& t = static_cast<const T&>(*this);
        auto index = t.array().atIdentityHash(item);
        return !t.array().at(index).isNil();
    }

protected:
    static T make(ThreadContext* context, int32_t capacity) {
        auto set = T::alloc(context);
        set.initToNil();
        set.setArray(Array::newClear(context, (capacity * 3) / 2));
        set.setSize(0);
        return set;
    }
};

class IdentitySet : public IdentitySetBase<IdentitySet, schema::IdentitySetSchema> {
public:
    IdentitySet(): IdentitySetBase<IdentitySet, schema::IdentitySetSchema>() {}
    explicit IdentitySet(schema::IdentitySetSchema* instance):
            IdentitySetBase<IdentitySet, schema::IdentitySetSchema>(instance) {}
    explicit IdentitySet(Slot instance): IdentitySetBase<IdentitySet, schema::IdentitySetSchema>(instance) {}
    ~IdentitySet() {}

    static IdentitySet makeIdentitySet(ThreadContext* context, int32_t capacity = 4) {
        return make(context, capacity);
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

// Support for Integers only right now, no ordering relationships exist beyond that.
class OrderedIdentitySet : public IdentitySetBase<OrderedIdentitySet, schema::OrderedIdentitySetSchema> {
public:
    OrderedIdentitySet(): IdentitySetBase<OrderedIdentitySet, schema::OrderedIdentitySetSchema>() {}
    explicit OrderedIdentitySet(schema::OrderedIdentitySetSchema* instance):
            IdentitySetBase<OrderedIdentitySet, schema::OrderedIdentitySetSchema>(instance) {}
    explicit OrderedIdentitySet(Slot instance):
            IdentitySetBase<OrderedIdentitySet, schema::OrderedIdentitySetSchema>(instance) {}
    ~OrderedIdentitySet() {}

    static OrderedIdentitySet makeIdentitySet(ThreadContext* context, int32_t capacity = 4) {
        auto set = make(context, capacity);
        set.setItems(TypedArray<Integer>());
        return set;
    }

    void add(ThreadContext* context, Slot item) {
        assert(item.isInt32());
        // Prevent duplicate additions to the items array.
        if (contains(item)) { return; }
        IdentitySetBase<OrderedIdentitySet, schema::OrderedIdentitySetSchema>::add(context, item);

        // Find the correct spot to insert this item, to maintain sorted order in the array. We're just directly
        // comparing Slot values, which doesn't make a lot of sense for non-numeric entries. Would need to route this
        // to the correct comparison function, perhaps after the refactor to always use type-generic comparisons.
        int32_t index = 0;
        for (; index < items().size(); ++index) {
            if (items().typedAt(index).int32() > item.getInt32()) { break; }
        }
        setItems(items().typedInsert(context, index, item));
    }

    // Items are always maintained in order.
    TypedArray<Integer> items() const { return TypedArray<Integer>(m_instance->items); }
    void setItems(TypedArray<Integer> a) { m_instance->items = a.slot(); }
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