#ifndef SRC_INCLUDE_HADRON_LINEAR_BLOCK_HPP_
#define SRC_INCLUDE_HADRON_LINEAR_BLOCK_HPP_

#include <vector>

namespace hadron {

struct LifetimeInterval;

struct LinearBlock {
    LinearBlock() = default;
    ~LinearBlock() = default;

    // Flattened list of all instructions, including Labels at the top of each block.
    std::vector<std::unique_ptr<hir::HIR>> instructions;
    // In-order list of each block.
    std::vector<int> blockOrder;
    // TODO: refactor to use Lifetime::Interval
    // index is block number, value is [start, end) of block instructions.
    std::vector<std::pair<size_t, size_t>> blockRanges;
    // index is value number
    std::vector<std::vector<LifetimeInterval>> valueLifetimes;
    // index is register number
    std::vector<std::vector<LifetimeInterval>> registerLifetimes;
    // index is spill slot number
    std::vector<std::vector<LifetimeInterval>> spillLifetimes;
    // Number of spill slots set after register allocation. We reserve spill slot 0 for temporary storage when breaking
    // copy cycles.
    size_t numberOfSpillSlots = 1;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_LINEAR_BLOCK_HPP_