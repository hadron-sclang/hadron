#ifndef SRC_COMPILER_INCLUDE_HADRON_LINEAR_FRAME_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_LINEAR_FRAME_HPP_

#include "hadron/hir/HIR.hpp"
#include "hadron/LifetimeInterval.hpp"
#include "hadron/lir/LIR.hpp"

#include <unordered_map>
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
    std::vector<lir::LabelID> blockOrder;
    // List iterators pointing at the first instruction in each block (which must be a LabelLIR)
    std::vector<LIRList::iterator> blockLabels;

    // We leave the ownership of LIR objects in a list, to allow for efficient reordering and deletion of instructions
    // during LIR optimization passes. But register allocation works best with line numbers. So after any optimization
    // passes LifetimeAnalyzer builds this vector of the final order of the LIR instructions.
    std::vector<lir::LIR*> lineNumbers;
    std::vector<std::pair<size_t, size_t>> blockRanges;

    // Index is value number
    std::vector<std::vector<LtIRef>> valueLifetimes;
    // Number of spill slots set after register allocation. We reserve spill slot 0 for temporary storage when breaking
    // copy cycles.
    size_t numberOfSpillSlots = 1;

    std::unordered_map<hir::ID, lir::VReg> hirToRegMap;

    // Convenience function, returns associated VReg in LIR or kInvalidVReg if no hir value found.
    lir::VReg hirToReg(hir::ID hirId);

    // If hirId is valid then we add to the mapping. Returns the assigned VReg or kInvalidVReg if no value assigned.
    lir::VReg append(hir::ID hirId, std::unique_ptr<lir::LIR> lir);
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_LINEAR_FRAME_HPP_