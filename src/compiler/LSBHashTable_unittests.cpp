#include "hadron/LSBHashTable.hpp"

#include "hadron/Hash.hpp"

#include "doctest/doctest.h"

#include <memory>

namespace {
struct TableEntry {
    TableEntry(hadron::Hash h): hash(h) {}
    hadron::Hash hash;
    std::unique_ptr<TableEntry> next;
};

struct Table : public hadron::LSBHashTable<TableEntry> {
    Table(uint32_t size) : LSBHashTable(size) {}
    virtual ~Table() = default;
};
}

namespace hadron {

TEST_CASE("LSBHashTable Sizing") {
    SUBCASE("zero size") {
        Table table(0);
        CHECK(table.tableSize == 0);
        CHECK(table.hashMask == 0);
        CHECK(table.table == nullptr);
    }
    SUBCASE("size one") {
        Table table(1);
        CHECK(table.tableSize == 1);
        CHECK(table.hashMask == 0);
        CHECK(table.table != nullptr);
    }
    SUBCASE("almost power of two") {
        Table table(126);
        CHECK(table.tableSize == 128);
        CHECK(table.hashMask == 127);
        CHECK(table.table != nullptr);
    }
    SUBCASE("exactly power of two") {
        Table table(4096);
        CHECK(table.tableSize == 4096);
        CHECK(table.hashMask == 4095);
        CHECK(table.table != nullptr);
    }
    SUBCASE("just above a power of two") {
        Table table(9);
        CHECK(table.tableSize == 16);
        CHECK(table.hashMask == 15);
        CHECK(table.table != nullptr);
    }
}

TEST_CASE("LSBHashTable addEntry") {
    SUBCASE("full table forward") {
        Table table(4);
        for (Hash i = 0; i < 16; ++i) {
            table.addEntry(std::make_unique<TableEntry>(i));
        }
        CHECK(table.numberOfEntries == 16);
        // Expecting all 4 table entries to be full with 4 entries per in sorted order.
        for (uint32_t i = 0; i < 4; ++i) {
            REQUIRE(table.table[i] != nullptr);
            const TableEntry* entry = table.table[i].get();
            CHECK(entry->hash == i);
            REQUIRE(entry->next != nullptr);
            entry = entry->next.get();
            CHECK(entry->hash == (i + 4));
            REQUIRE(entry->next != nullptr);
            entry = entry->next.get();
            CHECK(entry->hash == (i + 8));
            REQUIRE(entry->next != nullptr);
            entry = entry->next.get();
            CHECK(entry->hash == (i + 12));
            CHECK(entry->next == nullptr);
        }
    }
    SUBCASE("full table backwards") {
        Table table(4);
        for (Hash i = 16; i > 0; --i) {
            table.addEntry(std::make_unique<TableEntry>(i - 1));
        }
        CHECK(table.numberOfEntries == 16);
        // Expecting all 4 table entries to be full with 4 entries per in sorted order.
        for (uint32_t i = 0; i < 4; ++i) {
            REQUIRE(table.table[i] != nullptr);
            const TableEntry* entry = table.table[i].get();
            CHECK(entry->hash == i);
            REQUIRE(entry->next != nullptr);
            entry = entry->next.get();
            CHECK(entry->hash == (i + 4));
            REQUIRE(entry->next != nullptr);
            entry = entry->next.get();
            CHECK(entry->hash == (i + 8));
            REQUIRE(entry->next != nullptr);
            entry = entry->next.get();
            CHECK(entry->hash == (i + 12));
            CHECK(entry->next == nullptr);
        }
    }
}


} // namespace hadron