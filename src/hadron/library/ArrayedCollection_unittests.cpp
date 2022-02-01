#include "hadron/library/ArrayedCollection.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/ThreadContext.hpp"
#include "hadron/library/Symbol.hpp"

#include "doctest/doctest.h"

namespace hadron {

class ArrayedCollectionTestFixture {
public:
    ArrayedCollectionTestFixture():
        m_errorReporter(std::make_shared<ErrorReporter>()),
        m_runtime(std::make_unique<Runtime>(m_errorReporter)) {}
    virtual ~ArrayedCollectionTestFixture() = default;
protected:
    ThreadContext* context() { return m_runtime->context(); }
private:
    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<Runtime> m_runtime;
};

TEST_CASE_FIXTURE(ArrayedCollectionTestFixture, "SymbolArray") {
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