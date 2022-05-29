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
}

} // namespace hadron