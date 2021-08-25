#ifndef SRC_HADRON_INCLUDE_LSB_HASH_TABLE_HPP_
#define SRC_HADRON_INCLUDE_LSB_HASH_TABLE_HPP_

#include "hadron/Hash.hpp"

#include <memory>

namespace hadron {

// While the C++ standard library provides a number of perfectly good hash tables we want something that is trivial and
// quick to access from machine code, to allow for efficient navigation of the hash table directly from Hadron machine
// code. This simple hash table uses the already-computed hashes and a mask to look at the least significant bits of the
// hash, which so masked is the index into the array. Each array alement contains a linked list of colliding LSB hashes.
// Therefore, template types T must contain a hash element, as well as a std::unique_ptr<T> next.
template<typename T>
struct LSBHashTable {
    LSBHashTable(): tableSize(0), hashMask(0), numberOfEntries(0) { }
    // Create an empty hash table, size will be rounded up to next power of two.
    LSBHashTable(uint32_t size): tableSize(0), hashMask(0), numberOfEntries(0) { resize(size); }

    virtual ~LSBHashTable() = default;

    // Entry with same hash will end up in the collision list next to each other. entry->next will be clobbered.
    void addEntry(std::unique_ptr<T> entry) {
        int offset = static_cast<uint32_t>(hashMask & entry->hash);
        if (!table[offset]) {
            entry->next = nullptr;
            table[offset] = std::move(entry);
        } else if (entry->hash < table[offset]->hash) {
            entry->next = std::move(table[offset]);
            table[offset] = std::move(entry);
        } else {
            // Insert ourselves in the linked list in sorted order.
            T* hashElement = table[offset].get();
            while (hashElement->next != nullptr && hashElement->next->hash < entry->hash) {
                hashElement = hashElement->next.get();
            }
            entry->next = std::move(hashElement->next);
            hashElement->next = std::move(entry);
        }
        ++numberOfEntries;
    }

    // Returns a pointer to the first matching entry or nullptr if not found.
    const T* findElement(Hash hash) const {
        int offset = static_cast<uint32_t>(hashMask & hash);
        const T* entry = table[offset].get();
        while (entry) {
            if (hash == entry->hash) {
                return entry;
            } else if (hash < entry->hash) {
                entry = entry->next.get();
            } else {
                return nullptr;
            }
        }
        return entry;
    }

    // Removes the first matching entry found from the hash table and returns it, or returns nullptr if not found.
    std::unique_ptr<T> removeElement(Hash hash) const {
        int offset = static_cast<uint32_t>(hashMask & hash);
        if (!table[offset]) {
            return nullptr;
        } else if (table[offset]->hash == hash) {
            std::unique_ptr<T> entry = std::move(table[offset]);
            table[offset] = std::move(entry->next);
            entry->next = nullptr;
            --numberOfEntries;
            return std::move(entry);
        }
        T* priorElement = table[offset].get();
        while (priorElement->next) {
            if (hash == priorElement->next.hash) {
                std::unique_ptr<T> entry = std::move(priorElement->next);
                priorElement->next = std::move(entry->next);
                entry->next = nullptr;
                --numberOfEntries;
                return std::move(entry);
            } else if (hash < priorElement->next.hash) {
                priorElement = priorElement->next.get();
            } else {
                return nullptr;
            }
        }

        return nullptr;
    }

    // For a new table, allocates memory and sets up the mask for the appropriate size table. For an existing table,
    // will rehash and move every table entry into the new size table.
    void resize(uint32_t size) {
        uint32_t oldTableSize = tableSize;

        // From Hacker's Delight 2nd. Ed, by Henry S Warren, Jr. Compute the least power of two > size.
        size = size - 1;
        size = size | (size >> 1);
        size = size | (size >> 2);
        size = size | (size >> 4);
        size = size | (size >> 8);
        size = size | (size >> 16);
        tableSize = size + 1;

        // Since tableSize is a single bit we know the mask for that is just that value minus 1.
        hashMask = tableSize > 0 ? tableSize - 1 : 0;

        // Save the old table for moving entries.
        std::unique_ptr<std::unique_ptr<T> []> oldTable = std::move(table);

        // Allocate memory for the table if needed.
        if (tableSize == 0) {
            table = nullptr;
            return;
        }

        table = std::make_unique<std::unique_ptr<T> []>(tableSize);
        for (uint32_t i = 0; i < oldTableSize; ++i) {
            std::unique_ptr<T> entry = std::move(oldTable[i]);
            while (entry) {
                std::unique_ptr<T> nextEntry = std::move(entry->next);
                entry->next = nullptr;
                addEntry(std::move(entry));
                entry = std::move(nextEntry);
            }
        }
    }

    uint32_t tableSize;
    Hash hashMask;
    uint32_t numberOfEntries;
    std::unique_ptr<std::unique_ptr<T> []> table;
};


} // namespace hadron

#endif // SRC_HADRON_INCLUDE_LSB_HASH_TABLE_HPP_