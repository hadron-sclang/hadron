#ifndef SRC_HADRON_LIBRARY_DICTIONARY_HPP_
#define SRC_HADRON_LIBRARY_DICTIONARY_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/Set.hpp"
#include "hadron/schema/Common/Collections/DictionarySchema.hpp"

namespace hadron { namespace library {

template <typename T, typename S> class Dictionary : public Set<T, S> {
public:
    Dictionary(): Set<T, S>() { }
    explicit Dictionary(S* instance): Set<T, S>(instance) { }
    explicit Dictionary(Slot instance): Set<T, S>(instance) { }
    ~Dictionary() { }
};

class IdentityDictionary : public Dictionary<IdentityDictionary, schema::IdentityDictionarySchema> {
public:
    IdentityDictionary(): Dictionary<IdentityDictionary, schema::IdentityDictionarySchema>() { }
    explicit IdentityDictionary(schema::IdentityDictionarySchema* instance):
        Dictionary<IdentityDictionary, schema::IdentityDictionarySchema>(instance) { }
    explicit IdentityDictionary(Slot instance):
        Dictionary<IdentityDictionary, schema::IdentityDictionarySchema>(instance) { }
    ~IdentityDictionary() { }

    static IdentityDictionary makeIdentityDictionary(ThreadContext* context, int32_t capacity = 4) {
        auto dict = IdentityDictionary::alloc(context);
        // Array should be 3 / 2 the size of the number of keys, times 2 elements in the array per key/value pair means
        // we triple the size of the array to support |capacity| elements.
        auto arraySize = capacity * 3;
        // Always enforce even-sized arrays.
        if (arraySize % 2) {
            ++arraySize;
        }

        dict.setArray(Array::newClear(context, arraySize));
        dict.setSize(0);
        return dict;
    }

    void put(ThreadContext* context, Slot key, Slot value) {
        // Keys cannot be nil.
        assert(key);
        // Probing-style hash tables work better when they stay at most 2/3 full. Before insertion of this potentially
        // new element we check for a resize. Each element in the dictionary occupies two spaces in the array.
        if ((size() * 3) >= array().size()) {
            // TODO: repair unbounded size doubling.
            auto newDict = IdentityDictionary::makeIdentityDictionary(context, size() * 2);
            for (int32_t i = 0; i < array().size(); i += 2) {
                auto newKey = array().at(i);
                if (newKey) {
                    newDict.put(context, newKey, array().at(i + 1));
                }
            }
            // Copy the array from the new dictionary over our internal dictionary.
            setArray(newDict.array());
        }

        auto index = array().atIdentityHashInPairs(key);
        auto existingKey = array().at(index);
        if (existingKey) {
            assert(existingKey.identityHash() == key.identityHash());
        } else {
            setSize(size() + 1);
        }
        array().put(index, key);
        array().put(index + 1, value);
    }

    void putAll(ThreadContext* context, const IdentityDictionary dict) {
        for (int32_t i = 0; i < dict.array().size(); i += 2) {
            auto key = dict.array().at(i);
            if (key) {
                put(context, key, dict.array().at(i + 1));
            }
        }
    }

    Slot get(Slot key) {
        // keys cannot be nil.
        assert(key);
        auto index = array().atIdentityHashInPairs(key);
        return array().at(index + 1);
    }

    // Useful for iterating over contents by giving the keys. If |key| is nil, returns the *first* item in the array
    // or nil in an empty IdentityDictionary.
    Slot nextKey(Slot key) {
        int32_t index = key ? array().atIdentityHashInPairs(key) + 2 : 0;
        auto nextKey = Slot::makeNil();
        while (!nextKey && index < array().size()) {
            nextKey = array().at(index);
            index += 2;
        }

        return nextKey;
    }
};

template <typename K, typename V> class TypedIdentDict : public IdentityDictionary {
public:
    TypedIdentDict(): IdentityDictionary() { }
    explicit TypedIdentDict(schema::IdentityDictionarySchema* instance): IdentityDictionary(instance) { }
    explicit TypedIdentDict(Slot instance): IdentityDictionary(instance) { }
    ~TypedIdentDict() { }

    static TypedIdentDict<K, V> makeTypedIdentDict(ThreadContext* context, int32_t capacity = 4) {
        return TypedIdentDict<K, V>(IdentityDictionary::makeIdentityDictionary(context, capacity).slot());
    }

    void typedPut(ThreadContext* context, K key, V value) { put(context, key.slot(), value.slot()); }

    void typedPutAll(ThreadContext* context, TypedIdentDict<K, V> dict) { putAll(context, dict); }

    V typedGet(K key) { return V::wrapUnsafe(get(key.slot())); }
};


} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_DICTIONARY_HPP_