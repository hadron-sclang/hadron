#include "asmjit/x86/x86compiler.h"
#include "hadron/Generator.hpp"
#include "hadron/library/HadronHIR.hpp"

namespace hadron {

void Generator::buildFunction(asmjit::FuncSignature signature, std::vector<library::CFGBlock>& blocks,
                              library::TypedArray<library::BlockId> blockOrder) {
    asmjit::x86::Compiler compiler(&m_codeHolder);
    auto funcNode = compiler.addFunc(signature);

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

        for (int32_t i = 0; i < block.statements().size(); ++i) {
            auto hir = block.statements().typedAt(i);
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

            case library::ReadFromFrameHIR::nameHash(): {
                auto readFromFrameHIR = library::ReadFromFrameHIR(hir.slot());
            } break;

            case library::ReadFromThisHIR::nameHash(): {
                auto readFromThisHIR = library::ReadFromThisHIR(hir.slot());
            } break;

            case library::RouteToSuperclassHIR::nameHash(): {
                // TODO: super routing, perhaps with a different interrupt code to dispatch?
            } break;

            case library::StoreReturnHIR::nameHash(): {
                auto storeReturnHIR = library::StoreReturnHIR(hir.slot());
            } break;

            case library::WriteToClassHIR::nameHash(): {
                auto writeToClassHIR = library::WriteToClassHIR(hir.slot());
            } break;

            case library::WriteToFrameHIR::nameHash(): {
                auto writeToFrameHIR = library::WriteToFrameHIR(hir.slot());
            } break;

            case library::WriteToThisHIR::nameHash(): {
                auto writeToThisHIR = library::WriteToThisHIR(hir.slot());
            } break;

            default:
                assert(false); // missing case for hir
            }
        }
    }
}

} // namespace hadron