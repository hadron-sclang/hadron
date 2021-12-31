#define HADRON_PIPELINE_VALIDATE 1
#include "hadron/Pipeline.hpp"

#include "hadron/HIR.hpp"
#include "hadron/LifetimeInterval.hpp"
#include "hadron/LinearBlock.hpp"

#include "doctest/doctest.h"

namespace hadron {

// This is a mis-use of the "unit test" concept, in that no attempt is made to isolate the individual elements
// of the pipeline from each other. In order to help with that somewhat the pipeline is validating the output from each
// stage before applying the next stage. Additional breakout code for validation can happen in individual cases as
// needed. Many of the same snippets are also run as integration tests, but the difference here is that these tests are
// decidedly "hands-on," meaning they are expected to check values of individual fields and inspect internal state at
// any interesting stage of the pipeline.
TEST_CASE("Pipeline nil block") {
    Pipeline p;
    REQUIRE(p.compileBlock("nil"));
}

} // namespace hadron