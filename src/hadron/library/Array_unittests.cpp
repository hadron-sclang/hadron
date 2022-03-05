#include "hadron/library/Array.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/ThreadContext.hpp"

#include "doctest/doctest.h"

namespace hadron {

class ArrayTestFixture {
public:
    ArrayTestFixture():
        m_errorReporter(std::make_shared<ErrorReporter>()),
        m_runtime(std::make_unique<Runtime>(m_errorReporter)) {}
    virtual ~ArrayTestFixture() = default;
protected:
    ThreadContext* context() { return m_runtime->context(); }
private:
    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<Runtime> m_runtime;
};

TEST_CASE_FIXTURE(ArrayTestFixture, "Array") {
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
}

} // namespace hadron