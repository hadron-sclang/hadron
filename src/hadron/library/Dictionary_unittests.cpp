#include "hadron/library/Dictionary.hpp"

#include "hadron/library/LibraryTestFixture.hpp"
#include "hadron/library/Symbol.hpp"

#include "doctest/doctest.h"

namespace hadron {

TEST_CASE_FIXTURE(LibraryTestFixture, "IdentityDictionary") {
    SUBCASE("base case") {
        auto dict = library::IdentityDictionary::makeIdentityDictionary(context());
        CHECK_EQ(dict.size(), 0);
        CHECK(dict.get(Slot::makeInt32(0)).isNil());
    }

    SUBCASE("put symbols small") {
        auto dict = library::IdentityDictionary::makeIdentityDictionary(context());
        dict.put(context(), library::Symbol::fromView(context(), "a").slot(), Slot::makeInt32(0));
        dict.put(context(), library::Symbol::fromView(context(), "b").slot(), Slot::makeInt32(-1));
        dict.put(context(), library::Symbol::fromView(context(), "c").slot(), Slot::makeInt32(2));

        CHECK_EQ(dict.size(), 3);
        CHECK_EQ(dict.get(library::Symbol::fromView(context(), "b").slot()).getInt32(), -1);
        CHECK_EQ(dict.get(library::Symbol::fromView(context(), "a").slot()).getInt32(), 0);
        CHECK_EQ(dict.get(library::Symbol::fromView(context(), "c").slot()).getInt32(), 2);
        CHECK(dict.get(library::Symbol::fromView(context(), "d").slot()).isNil());
    }

    SUBCASE("put resize") {
        auto dict = library::IdentityDictionary::makeIdentityDictionary(context());
        for (int32_t i = 0; i < 128; ++i) {
            dict.put(context(), Slot::makeInt32(i), Slot::makeInt32(i * 2));
        }

        CHECK_EQ(dict.size(), 128);

        for (int32_t i = 0; i < 128; ++i) {
            CHECK_EQ(dict.get(Slot::makeInt32(i)).getInt32(), i * 2);
        }
    }

    SUBCASE("overwrites") {
        auto dict = library::IdentityDictionary::makeIdentityDictionary(context());
        for (int32_t i = 0; i < 128; ++i) {
            dict.put(context(), library::Symbol::fromView(context(), "overwrite").slot(), Slot::makeInt32(i));
            dict.put(context(), Slot::makeInt32(i), Slot::makeNil());
        }

        CHECK_EQ(dict.size(), 129);

        CHECK_EQ(dict.get(library::Symbol::fromView(context(), "overwrite").slot()), Slot::makeInt32(127));
    }
}

} // namespace hadron