#ifndef SRC_INCLUDE_HADRON_BLOCK_SERIALIZER_HPP_
#define SRC_INCLUDE_HADRON_BLOCK_SERIALIZER_HPP_

#include <memory>
#include <unordered_map>
#include <vector>

#include "hadron/HIR.hpp"
#include "hadron/Lifetime.hpp"

namespace hadron {

struct Block;
struct Frame;

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

// Serializes a Frame containing a control flow graph of blocks and HIR instructions into a single LinearBlock struct
// with LabelHIR instructions at the top of each block. Serialization order is in reverse postorder traversal, with
// all loops intact, which is a requirement for the lifetime analysis and register allocation stages of compilation.
class BlockSerializer {
public:
    BlockSerializer() = default;
    ~BlockSerializer() = default;

    // Destructively modify baseFrame to produce a single LinearBlock with blocks serialized in the required order.
    std::unique_ptr<LinearBlock> serialize(std::unique_ptr<Frame> baseFrame);

private:
    // Map of block number to Block struct, useful when recursing through control flow graph.
    std::unordered_map<int, Block*> m_blockMap;

    void orderBlocks(Block* block, std::vector<int>& blockOrder);
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_BLOCK_SERIALIZER_HPP_