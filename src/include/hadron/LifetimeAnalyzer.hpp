#ifndef SRC_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_
#define SRC_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_

#include "hadron/HIR.hpp"

#include <list>
#include <memory>
#include <unordered_map>
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

    // Destructively modify baseFrame to produce a single LinearBlock with 
    std::unique_ptr<LinearBlock> buildLifetimes(std::unique_ptr<Frame> baseFrame);

private:
    void orderBlocks(Block* block);

    std::vector<int> m_blockOrder;
    std::unordered_map<int, Block*> m_blocks;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_