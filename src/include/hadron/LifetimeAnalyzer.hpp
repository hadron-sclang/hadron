#ifndef SRC_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_
#define SRC_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_

#include "hadron/HIR.hpp"
#include "hadron/Lifetime.hpp"

#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

namespace hadron {

struct Block;
struct Frame;

// A lifetime for SSA form code consists of a beginning assignment time (where all times are instruction numbers in the
// linear block), a sorted list of use times, and a set of pairs of [start, end] times where the value is "live"
struct LinearBlock {
    // map of variable number to lifetime interval set.
    std::unordered_map<size_t, Lifetime> lifetimes;
    // Flattened list of all instructions, including Labels at the top of each block.
    std::vector<std::unique_ptr<hir::HIR>> instructions;

    // In-order list of each block.
    std::vector<int> blockOrder;
    // key is block number, value is [start, end] of block instructions.
    std::unordered_map<int, std::pair<size_t, size_t>> blockRanges;
};

class LifetimeAnalyzer {
public:
    LifetimeAnalyzer();
    ~LifetimeAnalyzer();

    // Destructively modify baseFrame to produce a single LinearBlock with all value Lifetimes computed.
    std::unique_ptr<LinearBlock> buildLifetimes(std::unique_ptr<Frame> baseFrame);

private:
    void orderBlocks(Block* block, std::vector<int>& blockOrder, std::unordered_map<int, Block*>& blockMap);
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_