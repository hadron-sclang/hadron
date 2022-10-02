#include "hadron/Generator.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/library/HadronHIR.hpp"

#include "spdlog/spdlog.h"

#include <algorithm>

namespace hadron {

SCMethod Generator::buildFunction(ThreadContext* context, asmjit::FuncSignature signature,
                                  std::vector<library::CFGBlock>& blocks,
                                  library::TypedArray<library::BlockId> blockOrder) {
    asmjit::CodeHolder codeHolder;
    codeHolder.init(m_jitRuntime.environment());

    // Create compiler object and attach to code space
    asmjit::a64::Compiler compiler(&codeHolder);
    auto funcNode = compiler.addFunc(signature);

    asmjit::a64::Gp contextReg = compiler.newGp(asmjit::TypeId::kIntPtr);
    funcNode->setArg(0, contextReg);
    asmjit::a64::Gp framePointerReg = compiler.newGp(asmjit::TypeId::kIntPtr);
    funcNode->setArg(1, framePointerReg);
    asmjit::a64::Gp stackPointerReg = compiler.newGp(asmjit::TypeId::kIntPtr);
    funcNode->setArg(2, stackPointerReg);

    std::vector<asmjit::Label> blockLabels(blocks.size());
    // TODO: maybe lazily create, as some blocks may have been deleted?
    std::generate(blockLabels.begin(), blockLabels.end(), [&compiler]() { return compiler.newLabel(); });

    std::vector<asmjit::a64::Gp> vRegs(blocks[0].frame().values().size());
    std::generate(vRegs.begin(), vRegs.end(), [&compiler]() { return compiler.newGp(asmjit::TypeId::kUInt64); });

    for (int32_t i = 0; i < blockOrder.size(); ++i) {
        auto blockNumber = blockOrder.typedAt(i).int32();
        auto block = blocks[blockNumber];

        // Bind label to current position in the code.
        compiler.bind(blockLabels[blockNumber]);

        // TODO: phis
        assert(block.phis().size() == 0);

        for (int32_t j = 0; j < block.statements().size(); ++j) {
            auto hir = block.statements().typedAt(j);
            switch (hir.className()) {
            case library::BlockLiteralHIR::nameHash(): {
                auto blockLiteralHIR = library::BlockLiteralHIR(hir.slot());
                auto funcPointerReg = compiler.newGp(asmjit::TypeId::kIntPtr);
                auto functionPointer = compiler.newGp(asmjit::TypeId::kUIntPtr);
                compiler.mov(functionPointer, (uint64_t)Generator::newFunction);
                asmjit::InvokeNode* invokeNode = nullptr;
                compiler.invoke(
                    &invokeNode, functionPointer,
                    asmjit::FuncSignatureT<schema::FunctionSchema*, ThreadContext*>(asmjit::CallConvId::kHost));
                invokeNode->setArg(0, contextReg);
                invokeNode->setRet(0, funcPointerReg);

                // Do something with the func Pointer? Or just have the function build it?
                break;
            }

            case library::BranchHIR::nameHash(): {
                auto branchHIR = library::BranchHIR(hir.slot());
                compiler.b(blockLabels[branchHIR.blockId().int32()]);
            } break;

            case library::BranchIfTrueHIR::nameHash(): {
                auto branchIfTrueHIR = library::BranchIfTrueHIR(hir.slot());
                compiler.cmp(vRegs[branchIfTrueHIR.condition().int32()], asmjit::Imm(Slot::makeBool(true).asBits()));
                compiler.b_eq(blockLabels[branchIfTrueHIR.blockId().int32()]);
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
                // Save arguments to the stack
                int32_t offset = 0;
                for (int32_t k = 0; k < messageHIR.arguments().size(); ++k) {
                    auto dest = asmjit::a64::ptr(stackPointerReg, offset);
                    compiler.str(vRegs[messageHIR.arguments().typedAt(k).int32()], dest);
                    offset += kSlotSize;
                }
                for (int32_t k = 0; k < messageHIR.keywordArguments().size(); ++k) {
                    auto dest = asmjit::a64::ptr(stackPointerReg, offset);
                    compiler.str(vRegs[messageHIR.keywordArguments().typedAt(k).int32()], dest);
                    offset += kSlotSize;
                }

                // a64 compiler always wants function pointers in a register, otherwise the generated code will crash.
                asmjit::a64::Gp functionPointer = compiler.newUIntPtr();
                compiler.mov(functionPointer, (uint64_t)ClassLibrary::dispatch);

                asmjit::InvokeNode* invokeNode = nullptr;
                compiler.invoke(&invokeNode, functionPointer,
                                asmjit::FuncSignatureT<uint64_t, ThreadContext*, Hash, int32_t, int32_t,
                                                       schema::FramePrivateSchema*, Slot*>(asmjit::CallConvId::kHost));
                invokeNode->setArg(0, contextReg);
                invokeNode->setArg(1, asmjit::Imm(messageHIR.selector(context).hash()));
                invokeNode->setArg(2, asmjit::Imm(messageHIR.arguments().size()));
                invokeNode->setArg(3, asmjit::Imm(messageHIR.keywordArguments().size() / 2));
                invokeNode->setArg(4, framePointerReg);
                invokeNode->setArg(5, stackPointerReg);
                invokeNode->setRet(0, vRegs[messageHIR.id().int32()]);
            } break;

            case library::MethodReturnHIR::nameHash(): {
                auto methodReturnHIR = library::MethodReturnHIR(hir.slot());
                compiler.ret(vRegs[methodReturnHIR.returnValue().int32()]);
            } break;

            case library::PhiHIR::nameHash(): {
                // Shouldn't encounter phis in block statements.
                assert(false);
            } break;

            case library::ReadFromClassHIR::nameHash(): {
                auto readFromClassHIR = library::ReadFromClassHIR(hir.slot());
                assert(false);
            } break;

            case library::ReadFromContextHIR::nameHash(): {
                auto readFromContextHIR = library::ReadFromContextHIR(hir.slot());
                assert(false);
            } break;

            case library::ReadFromFrameHIR::nameHash(): {
                auto readFromFrameHIR = library::ReadFromFrameHIR(hir.slot());
                auto fp = readFromFrameHIR.frameId() ? vRegs[readFromFrameHIR.frameId().int32()] : framePointerReg;
                auto src = asmjit::a64::ptr(
                    fp, sizeof(schema::FramePrivateSchema) + ((readFromFrameHIR.frameIndex() - 1) * kSlotSize));
                compiler.ldr(vRegs[readFromFrameHIR.id().int32()], src);
            } break;

            case library::ReadFromThisHIR::nameHash(): {
                auto readFromThisHIR = library::ReadFromThisHIR(hir.slot());
                assert(false);
            } break;

            case library::RouteToSuperclassHIR::nameHash(): {
                // TODO: super routing, perhaps with a different interrupt code to dispatch?
                assert(false);
            } break;

            case library::WriteToClassHIR::nameHash(): {
                auto writeToClassHIR = library::WriteToClassHIR(hir.slot());
                assert(false);
            } break;

            case library::WriteToFrameHIR::nameHash(): {
                auto writeToFrameHIR = library::WriteToFrameHIR(hir.slot());
                assert(false);
            } break;

            case library::WriteToThisHIR::nameHash(): {
                auto writeToThisHIR = library::WriteToThisHIR(hir.slot());
                assert(false);
            } break;

            default:
                assert(false); // missing case for hir
            }
        }
    }

    compiler.endFunc();
    compiler.finalize();

    SCMethod method;
    auto error = m_jitRuntime.add(&method, &codeHolder);
    if (error)
        return nullptr;

    return method;
}

} // namespace hadron