#ifndef SRC_COMPILER_INCLUDE_HADRON_LINEAR_BLOCK_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_LINEAR_BLOCK_HPP_

#include "hadron/LifetimeInterval.hpp"
#include "hadron/lir/LIR.hpp"

#include <vector>

namespace hadron {

struct LinearBlock {
    LinearBlock() = default;
    ~LinearBlock() = default;

    // Flattened list of all instructions, including Labels at the top of each block.
    std::vector<std::unique_ptr<lir::LIR>> instructions;
    // In-order list of each block.
    std::vector<int> blockOrder;
    // TODO: refactor to use LiveRange?
    // index is block number, value is [start, end) of block instructions.
    std::vector<std::pair<size_t, size_t>> blockRanges;
    // Index is value number
    std::vector<std::vector<LtIRef>> valueLifetimes;
    // Number of spill slots set after register allocation. We reserve spill slot 0 for temporary storage when breaking
    // copy cycles.
    size_t numberOfSpillSlots = 1;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_LINEAR_BLOCK_HPP_