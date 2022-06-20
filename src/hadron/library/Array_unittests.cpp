#include "hadron/library/Array.hpp"

#include "hadron/library/LibraryTestFixture.hpp"

#include "doctest/doctest.h"

namespace hadron {

TEST_CASE_FIXTURE(LibraryTestFixture, "Array") {
    SUBCASE("add") {
        auto array = library::Array();

        array = array.add(context(), Slot::makeNil());
        REQUIRE_EQ(array.size(), 1);
        CHECK_EQ(array.at(0), Slot::makeNil());

        array = array.add(context(), Slot::makeInt32(-1));
        REQUIRE_EQ(array.size(), 2);
        CHECK_EQ(array.at(0), Slot::makeNil());
        CHECK_EQ(array.at(1), Slot::makeInt32(-1));
    }

    SUBCASE("addAll") {
        auto array = library::Array();

        // addAll with 2 empty arrays.
        auto emptyArray = library::Array();
        array = array.addAll(context(), emptyArray);
        REQUIRE_EQ(array.size(), 0);

        auto subArray = library::Array();
        subArray.add(context(), Slot::makeNil());
        subArray.add(context(), Slot::makeBool(false));
        subArray.add(context(), Slot::makeChar('|'));
        REQUIRE_EQ(subArray.size(), 3);

        array = array.addAll(context(), subArray);
        REQUIRE_EQ(array.size(), 3);

        array = array.addAll(context(), emptyArray);
        REQUIRE_EQ(array.size(), 3);

        array = array.addAll(context(), array);
        REQUIRE_EQ(array.size(), 6);

        // Force a capacity increase.
        array = array.addAll(context(), array);
        REQUIRE_EQ(array.size(), 12);
        array = array.addAll(context(), array);
        REQUIRE_EQ(array.size(), 24);
        array = array.addAll(context(), array);
        REQUIRE_EQ(array.size(), 48);
        array = array.addAll(context(), array);
        REQUIRE_EQ(array.size(), 96);
        array = array.addAll(context(), array);
        REQUIRE_EQ(array.size(), 192);

        REQUIRE_EQ(array.className(), library::Array::nameHash());
    }

    SUBCASE("newClear") {
        auto array = library::Array::newClear(context(), 25);
        REQUIRE_EQ(array.size(), 25);
        for (int32_t i = 0; i < 25; ++i) {
            CHECK(array.at(i).isNil());
        }
    }

    SUBCASE("insert") {
        auto array = library::Array();

        array = array.insert(context(), 0, Slot::makeInt32(1));
        REQUIRE_EQ(array.size(), 1);
        CHECK_EQ(array.at(0), Slot::makeInt32(1));

        array = array.insert(context(), 1, Slot::makeInt32(3));
        REQUIRE_EQ(array.size(), 2);
        CHECK_EQ(array.at(0), Slot::makeInt32(1));
        CHECK_EQ(array.at(1), Slot::makeInt32(3));

        array = array.insert(context(), 0, Slot::makeInt32(0));
        REQUIRE_EQ(array.size(), 3);
        CHECK_EQ(array.at(0), Slot::makeInt32(0));
        CHECK_EQ(array.at(1), Slot::makeInt32(1));
        CHECK_EQ(array.at(2), Slot::makeInt32(3));

        array = array.insert(context(), 2, Slot::makeInt32(2));
        REQUIRE_EQ(array.size(), 4);
        CHECK_EQ(array.at(0), Slot::makeInt32(0));
        CHECK_EQ(array.at(1), Slot::makeInt32(1));
        CHECK_EQ(array.at(2), Slot::makeInt32(2));
        CHECK_EQ(array.at(3), Slot::makeInt32(3));

        for (int i = 0; i < 96; ++i) {
            array = array.insert(context(), 2, Slot::makeInt32(-i));
        }
        REQUIRE_EQ(array.size(), 100);
        CHECK_EQ(array.at(0), Slot::makeInt32(0));
        CHECK_EQ(array.at(1), Slot::makeInt32(1));
        for (int i = 0; i < 96; ++i) {
            CHECK_EQ(array.at(97 - i).getInt32(), -i);
        }
        CHECK_EQ(array.at(98), Slot::makeInt32(2));
        CHECK_EQ(array.at(99), Slot::makeInt32(3));
    }

    SUBCASE("removeAt") {
        #error test me
    }
}

} // namespace hadron