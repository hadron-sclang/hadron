#include "hadron/library/Set.hpp"

#include "hadron/library/LibraryTestFixture.hpp"
#include "hadron/library/Symbol.hpp"

#include "doctest/doctest.h"
#include "fmt/format.h"

namespace hadron {

TEST_CASE_FIXTURE(LibraryTestFixture, "IdentitySet") {
    SUBCASE("base case") {
        auto set = library::IdentitySet::makeIdentitySet(context());
        CHECK_EQ(set.size(), 0);
        CHECK(!set.contains(Slot::makeInt32(0)));
    }

    SUBCASE("add symbols small") {
        auto set = library::IdentitySet::makeIdentitySet(context());
        set.add(context(), library::Symbol::fromView(context(), "x").slot());
        set.add(context(), library::Symbol::fromView(context(), "y").slot());
        set.add(context(), library::Symbol::fromView(context(), "z").slot());

        CHECK_EQ(set.size(), 3);
        CHECK(set.contains(library::Symbol::fromView(context(), "z").slot()));
        CHECK(set.contains(library::Symbol::fromView(context(), "y").slot()));
        CHECK(set.contains(library::Symbol::fromView(context(), "x").slot()));
        CHECK(!set.contains(library::Symbol::fromView(context(), "w").slot()));
    }

    SUBCASE("add resize") {
        auto set = library::IdentitySet::makeIdentitySet(context());
        for (int32_t i = 0; i < 255; i += 5) {
            set.add(context(), Slot::makeInt32(i));
            set.add(context(), Slot::makeFloat(static_cast<double>(i + 1)));
            set.add(context(), library::Symbol::fromView(context(), fmt::format("{}", i + 2)).slot());
            set.add(context(), Slot::makeChar(static_cast<char>(i + 3)));
            // Booleans will overwrite each other.
            set.add(context(), Slot::makeBool((i + 4) % 2));
        }

        CHECK_EQ(set.size(), 255 - (255 / 5) + 2);

        for (int32_t i = 0; i < 255; i += 5) {
            CHECK(set.contains(Slot::makeInt32(i)));
            CHECK(set.contains(Slot::makeFloat(static_cast<double>(i + 1))));
            CHECK(set.contains(library::Symbol::fromView(context(), fmt::format("{}", i + 2)).slot()));
            CHECK(set.contains(Slot::makeChar(static_cast<char>(i + 3))));
            CHECK(set.contains(Slot::makeBool((i + 4) % 2)));
        }
    }

    SUBCASE("ordered set") {
        auto set = library::OrderedIdentitySet::makeIdentitySet(context());
        set.add(context(), Slot::makeInt32(200));
        set.add(context(), Slot::makeInt32(-5));
        set.add(context(), Slot::makeInt32(0));
        set.add(context(), Slot::makeInt32(99));
        set.add(context(), Slot::makeInt32(-5));
        set.add(context(), Slot::makeInt32(-98));
        set.add(context(), Slot::makeInt32(4));
        set.add(context(), Slot::makeInt32(200));

        REQUIRE_EQ(set.size(), 6);
        REQUIRE_EQ(set.items().size(), 6);

        CHECK(set.contains(Slot::makeInt32(-98)));
        CHECK_EQ(set.items().at(0).getInt32(), -98);

        CHECK(set.contains(Slot::makeInt32(-5)));
        CHECK_EQ(set.items().at(1).getInt32(), -5);

        CHECK(set.contains(Slot::makeInt32(0)));
        CHECK_EQ(set.items().at(2).getInt32(), 0);

        CHECK(set.contains(Slot::makeInt32(4)));
        CHECK_EQ(set.items().at(3).getInt32(), 4);

        CHECK(set.contains(Slot::makeInt32(99)));
        CHECK_EQ(set.items().at(4).getInt32(), 99);

        CHECK(set.contains(Slot::makeInt32(200)));
        CHECK_EQ(set.items().at(5).getInt32(), 200);
    }
}

} // namespace hadron