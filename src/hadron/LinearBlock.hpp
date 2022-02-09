#ifndef SRC_COMPILER_INCLUDE_HADRON_LINEAR_BLOCK_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_LINEAR_BLOCK_HPP_

#include "hadron/LifetimeInterval.hpp"
#include "hadron/lir/LIR.hpp"

#include <list>
#include <vector>

namespace hadron {

struct LinearBlock {
    LinearBlock() = default;
    ~LinearBlock() = default;

    using LIRList = std::list<std::unique_ptr<lir::LIR>>;
    // Flattened list of all instructions, including Labels at the top of each block.
    LIRList instructions;
    // vReg lookup table.
    std::vector<lir::LIR*> vRegs;
    // In-order list of each block.
    std::vector<int> blockOrder;
    // List iterators pointing at the first and last LIR instruction in each block.
    std::vector<std::pair<LIRList::iterator, LIRList::iterator>> blockRanges;
    // Index is value number
    std::vector<std::vector<LtIRef>> valueLifetimes;
    // Number of spill slots set after register allocation. We reserve spill slot 0 for temporary storage when breaking
    // copy cycles.
    size_t numberOfSpillSlots = 1;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_LINEAR_BLOCK_HPP_