#ifndef SRC_COMPILER_INCLUDE_HADRON_LINEAR_FRAME_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_LINEAR_FRAME_HPP_

#include "hadron/Block.hpp"
#include "hadron/LifetimeInterval.hpp"
#include "hadron/lir/LIR.hpp"

#include <vector>

namespace hadron {

struct LinearFrame {
    LinearFrame() = default;
    ~LinearFrame() = default;

    // Flattened list of all instructions, including Labels at the top of each block.
    LIRList instructions;
    // vReg lookup table.
    std::vector<LIRList::iterator> vRegs;
    // In-order list of each block.
    std::vector<Block::ID> blockOrder;
    // List iterators pointing at the first instruction in each block (which must be a LabelLIR)
    std::vector<LIRList::iterator> blockLabels;

    // We leave the owning list of LIR objects in a list, to allow for efficient reordering and deletion of instructions
    // during LIR optimization passes. But register allocation works best with line numbers. So after any optimization
    // passes LifetimeAnalyzer builds this vector of the final order of the LIR instructions.
    std::vector<lir::LIR*> lineNumbers;
    std::vector<std::pair<size_t, size_t>> blockRanges;

    // Index is value number
    std::vector<std::vector<LtIRef>> valueLifetimes;
    // Number of spill slots set after register allocation. We reserve spill slot 0 for temporary storage when breaking
    // copy cycles.
    size_t numberOfSpillSlots = 1;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_LINEAR_FRAME_HPP_