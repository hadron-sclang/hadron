#include "hadron/Generator.hpp"

#include "hadron/library/HadronHIR.hpp"

#include <algorithm>

namespace hadron {

void Generator::buildFunction(const library::CFGFrame /* frame */, asmjit::FuncSignature signature,
                              std::vector<library::CFGBlock>& blocks,
                              library::TypedArray<library::BlockId> blockOrder) {
    asmjit::x86::Compiler compiler(&m_codeHolder);
    auto funcNode = compiler.addFunc(signature);

    // Make room on stack frame for local variables.

    std::vector<asmjit::Label> blockLabels(blocks.size());
    // TODO: maybe lazily create, as some blocks may have been deleted?
    std::generate(blockLabels.begin(), blockLabels.end(), [&compiler]() { return compiler.newLabel(); });

    std::vector<asmjit::x86::Gpq> vRegs(blocks[0].frame().values().size());
    std::generate(vRegs.begin(), vRegs.end(), [&compiler]() { return compiler.newGpq(); });

    for (int32_t i = 0; i < blockOrder.size(); ++i) {
        auto blockNumber = blockOrder.typedAt(i).int32();
        auto block = blocks[blockNumber];

        // Bind label to current position in the code.
        compiler.bind(blockLabels[blockNumber]);

        // Phis can be resolved with creation of new virtual reg associated with phi followed by a jump to the
        // next HIR instruction
        // TODO: phis
        assert(block.phis().size() == 0);

        for (int32_t j = 0; j < block.statements().size(); ++j) {
            auto hir = block.statements().typedAt(j);
            switch (hir.className()) {
            case library::BlockLiteralHIR::nameHash():
                assert(false);
                break;

            case library::BranchHIR::nameHash(): {
                auto branchHIR = library::BranchHIR(hir.slot());
                compiler.jmp(blockLabels[branchHIR.blockId().int32()]);
            } break;

            case library::BranchIfTrueHIR::nameHash(): {
                auto branchIfTrueHIR = library::BranchIfTrueHIR(hir.slot());
                compiler.cmp(vRegs[branchIfTrueHIR.condition().int32()], asmjit::Imm(Slot::makeBool(true).asBits()));
                compiler.je(blockLabels[branchIfTrueHIR.blockId().int32()]);
            } break;

            case library::ConstantHIR::nameHash(): {
                auto constantHIR = library::ConstantHIR(hir.slot());
                compiler.mov(vRegs[constantHIR.id().int32()], asmjit::Imm(constantHIR.constant().asBits()));
            } break;

            case library::LoadOuterFrameHIR::nameHash(): {
                auto loadOuterFrameHIR = library::LoadOuterFrameHIR(hir.slot());
                assert(false);
            } break;

            case library::MessageHIR::nameHash(): {
                auto messageHIR = library::MessageHIR(hir.slot());
                assert(false);
            } break;

            // Need to adjust to include a return value.
            case library::MethodReturnHIR::nameHash(): {
                assert(false);
            }

            case library::PhiHIR::nameHash(): {
                // Shouldn't encounter phis in block statements.
                assert(false);
            } break;

            case library::ReadFromClassHIR::nameHash(): {
                auto readFromClassHIR = library::ReadFromClassHIR(hir.slot());
            } break;

            case library::ReadFromContextHIR::nameHash(): {
                auto readFromContextHIR = library::ReadFromContextHIR(hir.slot());
            } break;

            // TODO: args are no longer in frame
            case library::ReadFromFrameHIR::nameHash(): {
                auto readFromFrameHIR = library::ReadFromFrameHIR(hir.slot());
            } break;

            case library::ReadFromThisHIR::nameHash(): {
                auto readFromThisHIR = library::ReadFromThisHIR(hir.slot());
            } break;

            case library::RouteToSuperclassHIR::nameHash(): {
                // TODO: super routing, perhaps with a different interrupt code to dispatch?
            } break;

            case library::WriteToClassHIR::nameHash(): {
                auto writeToClassHIR = library::WriteToClassHIR(hir.slot());
            } break;

            case library::WriteToFrameHIR::nameHash(): {
                auto writeToFrameHIR = library::WriteToFrameHIR(hir.slot());

            } break;

            case library::WriteToThisHIR::nameHash(): {
                auto writeToThisHIR = library::WriteToThisHIR(hir.slot());
                auto dest = asmjit::x86::ptr(vRegs[writeToThisHIR.thisId().int32()], writeToThisHIR.index());
                compiler.mov(dest, vRegs[writeToThisHIR.toWrite().int32()]);
            } break;

            default:
                assert(false); // missing case for hir
            }
        }
    }
}

} // namespace hadron