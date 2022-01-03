#define HADRON_PIPELINE_VALIDATE 1
#include "hadron/Pipeline.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/HIR.hpp"
#include "hadron/LifetimeInterval.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/ThreadContext.hpp"

#include "doctest/doctest.h"

namespace hadron {

class PipelineTestFixture {
public:
    PipelineTestFixture():
        m_errorReporter(std::make_shared<ErrorReporter>()),
        m_runtime(std::make_unique<Runtime>(m_errorReporter)) {}
    virtual ~PipelineTestFixture() = default;
protected:
    ThreadContext* context() { return m_runtime->context(); }
private:
    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<Runtime> m_runtime;
};

// This is a mis-use of the "unit test" concept, in that no attempt is made to isolate the individual elements
// of the pipeline from each other. In order to help with that somewhat the pipeline is validating the output from each
// stage before applying the next stage. Additional breakout code for validation can happen in individual cases as
// needed. Many of the same snippets are also run as integration tests, but the difference here is that these tests are
// decidedly "hands-on," meaning they are expected to check values of individual fields and inspect internal state at
// any interesting stage of the pipeline.
TEST_CASE_FIXTURE(PipelineTestFixture, "Pipeline nil block") {
    Pipeline p;
    REQUIRE(p.compileBlock(context(), "nil"));
}

} // namespace hadron