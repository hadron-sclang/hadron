#ifndef SRC_HADRON_LIBRARY_DICTIONARY_HPP_
#define SRC_HADRON_LIBRARY_DICTIONARY_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/Set.hpp"
#include "hadron/schema/Common/Collections/DictionarySchema.hpp"

namespace hadron {
namespace library {

template<typename T, typename S>
class Dictionary : public Set<T, S> {
public:
    Dictionary(): Set<T, S>() {}
    explicit Dictionary(S* instance): Set<T, S>(instance) {}
    explicit Dictionary(Slot instance): Set<T, S>(instance) {}
    ~Dictionary() {}
};

class IdentityDictionary : public Dictionary<IdentityDictionary, schema::IdentityDictionarySchema> {
public:
    IdentityDictionary(): Dictionary<IdentityDictionary, schema::IdentityDictionarySchema>() {}
    explicit IdentityDictionary(schema::IdentityDictionarySchema* instance):
            Dictionary<IdentityDictionary, schema::IdentityDictionarySchema>(instance) {}
    explicit IdentityDictionary(Slot instance):
            Dictionary<IdentityDictionary, schema::IdentityDictionarySchema>(instance) {}
    ~IdentityDictionary() {}

    static IdentityDictionary makeIdentityDictionary(ThreadContext* context, int32_t capacity = 7) {
        auto dict = IdentityDictionary::alloc(context);
        // Array should be 3 / 2 the size of the number of keys, times 2 elements in the array per key/value pair means
        // we triple the size of the array to support |capacity| elements.
        auto arraySize = capacity * 3;
        // Always enforce even-sized arrays.
        if (arraySize % 2) { ++arraySize; }

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
                auto key = array().at(i);
                if (key) {
                    newDict.put(context, key, array().at(i + 1));
                }
            }
            // Copy the array from the new dictionary over our internal dictionary.
            setArray(newDict.array());
        }

        auto index = indexForKey(key);
        auto existingKey = array().at(index);
        if (existingKey) {
            assert(existingKey.identityHash() == key.identityHash());
        } else {
            setSize(size() + 1);
        }
        array().put(index, key);
        array().put(index + 1, value);
    }

    Slot get(Slot key) {
        // keys cannot be nil.
        assert(key);
        auto index = indexForKey(key);
        return array().at(index + 1);
    }

private:
    // The index in the array where an element with the same identityHash of |key| is, or the first nil element probed.
    int32_t indexForKey(Slot key) {
        auto hash = key.identityHash();
        // Keys are always at even indexes followed by their value pair at odd, so mask off the least significant bit to
        // compute even index.
        auto index = (hash % array().size()) & (~1);
        auto element = array().at(index);
        while (element && element.identityHash() != hash) {
            index = (index + 2) % array().size();
            element = array().at(index);
        }

        return index;
    }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_DICTIONARY_HPP_