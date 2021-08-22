#ifndef SRC_INCLUDE_HADRON_BLOCK_SERIALIZER_HPP_
#define SRC_INCLUDE_HADRON_BLOCK_SERIALIZER_HPP_

#include <list>
#include <memory>
#include <vector>

#include "hadron/HIR.hpp"
#include "hadron/LifetimeInterval.hpp"

namespace hadron {

struct Block;
struct Frame;

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
};

// Serializes a Frame containing a control flow graph of blocks and HIR instructions into a single LinearBlock struct
// with LabelHIR instructions at the top of each block. Serialization order is in reverse postorder traversal, with
// all loops intact, which is a requirement for the lifetime analysis and register allocation stages of compilation.
class BlockSerializer {
public:
    BlockSerializer() = default;
    ~BlockSerializer() = default;

    // Destructively modify baseFrame to produce a single LinearBlock with blocks serialized in the required order.
    std::unique_ptr<LinearBlock> serialize(std::unique_ptr<Frame> baseFrame, size_t numberOfRegisters);

private:
    // Map of block number to Block struct, useful when recursing through control flow graph.
    std::vector<Block*> m_blocks;

    void orderBlocks(Block* block, std::vector<int>& blockOrder);
    void reserveRegisters(LinearBlock* linearBlock);
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_BLOCK_SERIALIZER_HPP_