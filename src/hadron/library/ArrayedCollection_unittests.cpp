#include "hadron/library/ArrayedCollection.hpp"

#include "hadron/library/LibraryTestFixture.hpp"
#include "hadron/library/Symbol.hpp"

#include "doctest/doctest.h"

namespace hadron {

TEST_CASE_FIXTURE(LibraryTestFixture, "SymbolArray") {
    SUBCASE("access") {
        auto array = library::SymbolArray::arrayAlloc(context(), 3);
        array.add(context(), library::Symbol::fromView(context(), "a"));
        array.add(context(), library::Symbol::fromView(context(), "b"));
        array.add(context(), library::Symbol::fromView(context(), "c"));
        CHECK_EQ(array.at(2), library::Symbol::fromView(context(), "c"));
        CHECK_EQ(array.at(1), library::Symbol::fromView(context(), "b"));
        CHECK_EQ(array.at(0), library::Symbol::fromView(context(), "a"));
    }
}

} // namespace hadron