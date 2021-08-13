#ifndef SRC_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_
#define SRC_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_

#include "hadron/HIR.hpp"

#include <memory>
#include <vector>

namespace hadron {

struct Block;
struct Frame;

// A lifetime for SSA form code consists of a beginning assignment time (where all times are instruction numbers in the
// linear block), a sorted list of use times, and a set of pairs of [start, end] times where the value is "live"
struct Lifetime {
    size_t assignmentTime;
    std::vector<size_t> useTimes;
    std::vector<std::pair<size_t, size_t>> liveRanges;
};

struct LinearBlock {
    std::vector<Lifetime> lifetimes;
    std::vector<std::unique_ptr<hir::HIR>> instructions;
};

class LifetimeAnalyzer {
public:
    LifetimeAnalyzer();
    ~LifetimeAnalyzer();

    bool buildLifetimes(const Frame* baseFrame);

private:
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_