#include "asmjit/x86/x86compiler.h"
#include "hadron/Generator.hpp"

namespace hadron {

void Generator::buildFunction(asmjit::FuncSignature signature, std::vector<library::CFGBlock>& blocks,
                              library::TypedArray<library::BlockId> blockOrder) {
    asmjit::x86::Compiler compiler(&m_codeHolder);
    auto funcNode = compiler.addFunc(signature);

    std::vector<asmjit::Label> blockLabels(blocks.size());
    // TODO: maybe lazily create, as some blocks may have been deleted?
    std::generate(blockLabels.begin(), blockLabels.end(), [&compiler]() { return compiler.newLabel(); });

    for (int32_t i = 0; i < blockOrder.size(); ++i) {
        auto blockNumber = blockOrder.typedAt(i).int32();
        auto block = blocks[blockNumber];

        // Bind label to current position in the code.
        compiler.bind(blockLabels[blockNumber]);

        // Phis can be resolved with creation of new virtual reg associated with phi followed by a jump to the
        // next HIR instruction
        // TODO: phis
        assert(block.phis().size() == 0);
    }

}

} // namespace hadron