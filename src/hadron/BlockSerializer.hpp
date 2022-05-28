#ifndef SRC_COMPILER_INCLUDE_HADRON_BLOCK_SERIALIZER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_BLOCK_SERIALIZER_HPP_

#include "hadron/library/HadronCFG.hpp"
#include "hadron/lir/LIR.hpp"

#include <list>
#include <memory>
#include <vector>

namespace hadron {

struct LinearFrame;

// Serializes a Frame containing a control flow graph of blocks and HIR instructions into a single LinearFrame struct
// with LabelHIR instructions at the top of each block. Serialization order is in reverse postorder traversal, with
// all loops intact, which is a requirement for the lifetime analysis and register allocation stages of compilation.
class BlockSerializer {
public:
    BlockSerializer() = default;
    ~BlockSerializer() = default;

    // Produce a single LinearFrame with blocks serialized in the required order.
    std::unique_ptr<LinearFrame> serialize(const Frame* frame);

private:

    // Does the recursive postorder traversal of the blocks and saves the output in |blockOrder|.
    void orderBlocks(Block* block, std::vector<Block*>& blockPointers, std::vector<lir::LabelID>& blockOrder);
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_BLOCK_SERIALIZER_HPP_