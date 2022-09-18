#ifndef SRC_COMPILER_INCLUDE_HADRON_BLOCK_SERIALIZER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_BLOCK_SERIALIZER_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/HadronCFG.hpp"
#include "hadron/library/HadronHIR.hpp"
#include "hadron/library/HadronLinearFrame.hpp"
#include "hadron/library/HadronLIR.hpp"
#include "hadron/library/Integer.hpp"

#include <list>
#include <memory>
#include <vector>

namespace hadron {

struct ThreadContext;

// Serializes a Frame containing a control flow graph of blocks and HIR instructions into a single LinearFrame struct
// with LabelHIR instructions at the top of each block. Serialization order is in reverse postorder traversal, with
// all loops intact, which is a requirement for the lifetime analysis and register allocation stages of compilation.
class BlockSerializer {
public:
    BlockSerializer() = default;
    ~BlockSerializer() = default;

    // Produce a single LinearFrame with blocks serialized in the required order.
    library::LinearFrame serialize(ThreadContext* context, const library::CFGFrame frame);

private:
    // Performs a recursive postorder traversal of the blocks and saves the output in |blockOrder|.
    void orderBlocks(ThreadContext* context, library::CFGBlock block, std::vector<library::CFGBlock>& blocks,
                     library::TypedArray<library::LabelId> blockOrder);

    void lower(ThreadContext* context, library::HIR hir, library::LinearFrame linearFrame,
               library::TypedArray<library::LIR> instructions);
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_BLOCK_SERIALIZER_HPP_