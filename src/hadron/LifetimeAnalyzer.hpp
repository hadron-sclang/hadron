#ifndef SRC_COMPILER_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_

namespace hadron {

struct LinearFrame;

// This is a literal implementation of the pseudocode described in the BuildIntervals algorithm of [RA5] in the
// Bibliography,  "Linear Scan Register Allocation on SSA Form" by C. Wimmer and M. Franz.
class LifetimeAnalyzer {
public:
    LifetimeAnalyzer() = default;
    ~LifetimeAnalyzer() = default;

    void buildLifetimes(LinearFrame* linearFrame);
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_