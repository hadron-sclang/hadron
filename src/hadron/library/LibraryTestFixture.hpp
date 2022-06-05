#ifndef SRC_HADRON_LIBRARY_LIBRARY_TEST_FIXTURE_HPP_
#define SRC_HADRON_LIBRARY_LIBRARY_TEST_FIXTURE_HPP_

#include "hadron/ErrorReporter.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/ThreadContext.hpp"

// For consumption by unittests only, a test fixture that creates a ThreadContext.
namespace hadron {

class LibraryTestFixture {
public:
    LibraryTestFixture():
        m_errorReporter(std::make_shared<ErrorReporter>()),
        m_runtime(std::make_unique<Runtime>(m_errorReporter)) {}
    virtual ~LibraryTestFixture() = default;
protected:
    ThreadContext* context() { return m_runtime->context(); }
private:
    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<Runtime> m_runtime;
};

} // namespace hadron

#endif // SRC_HADRON_LIBRARY_LIBRARY_TEST_FIXTURE_HPP_