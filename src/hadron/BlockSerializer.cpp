#include "hadron/BlockSerializer.hpp"

#include "hadron/library/Array.hpp"
#include "hadron/library/Function.hpp"
#include "hadron/library/HadronHIR.hpp"
#include "hadron/library/HadronLIR.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/ThreadContext.hpp"

namespace hadron {

library::LinearFrame BlockSerializer::serialize(ThreadContext* context, const library::CFGFrame frame) {
    // Prepare the LinearFrame for recording of value lifetimes.
    auto linearFrame = library::LinearFrame::make(context, frame.numberOfBlocks());

    // Map of block number (index) to Block struct, useful when recursing through control flow graph.
    std::vector<library::CFGBlock> blocks;
    blocks.resize(frame.numberOfBlocks(), library::CFGBlock());

    auto blockOrder = library::TypedArray<library::LabelId>::typedArrayAlloc(context, frame.numberOfBlocks());

    // Determine linear block order from reverse postorder traversal.
    orderBlocks(context, frame.rootScope().blocks().typedFirst(), blocks, blockOrder);
    blockOrder = blockOrder.typedReverse(context);
    linearFrame.setBlockOrder(blockOrder);

    auto instructions = library::TypedArray<library::LIR>::typedArrayAlloc(context);

    // Fill linear frame with LIR in computed order.
    for (int32_t i = 0; i < blockOrder.size(); ++i) {
        auto blockNumber = blockOrder.typedAt(i).int32();
        auto block = blocks[blockNumber];
        auto label = library::LabelLIR::make(context);
        label.setLabelId(block.id());
        label.setLoopReturnPredIndex(block.loopReturnPredIndex());

        auto predecessors = library::TypedArray<library::LabelId>::typedArrayAlloc(context,
                block.predecessors().size());
        for (int32_t i = 0; i < block.predecessors().size(); ++i) {
            auto pred = block.predecessors().typedAt(i);
            predecessors = predecessors.typedAdd(context, pred.id());
        }
        label.setPredecessors(predecessors);

        auto successors = library::TypedArray<library::LabelId>::typedArrayAlloc(context,
                block.successors().size());
        for (int32_t i = 0; i < block.successors().size(); ++i) {
            auto succ = block.successors().typedAt(i);
            successors = successors.typedAdd(context, succ.id());
        }
        label.setSuccessors(successors);

        // Lower any phis in the label.
        auto labelPhis = library::TypedArray<library::LIR>::typedArrayAlloc(context, block.phis().size());
        for (int32_t i = 0; i < block.phis().size(); ++i) {
            auto phi = block.phis().typedAt(i);

            auto phiLIR = library::PhiLIR::make(context);
            for (int32_t i = 0; i < phi.inputs().size(); ++i) {
                auto nvid = phi.inputs().typedAt(i);
                auto vReg = linearFrame.hirToReg(nvid);
                phiLIR.addInput(context, linearFrame.vRegs().typedAt(vReg.int32()));
            }
            linearFrame.append(context, phi.id(), phiLIR.toBase(), labelPhis);
        }
        label.setPhis(labelPhis);

        // Start the block with a label and then append all contained instructions.
        linearFrame.append(context, library::HIRId(), label.toBase(), instructions);
        linearFrame.blockLabels().typedPut(block.id(), label);

        for (int32_t i = 0; i < block.statements().size(); ++i) {
            auto hir = block.statements().typedAt(i);
            switch (hir.className()) {
            case library::BlockLiteralHIR::nameHash(): {
                auto blockLiteralHIR = library::BlockLiteralHIR(hir.slot());
                // If this BlockLiteral is being lowered that means that it was not inlined during optimization, and so
                // should be compiled down to bytecode as part of a FunctionDef, then the BlockLiteral becomes a call to
                // create a new Function object. The Materializer should have already compiled the FunctionDef and
                // provided it to the block.
                assert(!blockLiteralHIR.functionDef().isNil());

                // Interrupt to allocate memory for the Function object.
                linearFrame.append(context, library::HIRId(), library::InterruptLIR::makeInterrupt(context,
                        ThreadContext::InterruptCode::kAllocateMemory).toBase(), instructions);

                // TODO: actual useful stack frame offset
                auto functionVReg = linearFrame.append(context, blockLiteralHIR.id(),
                        library::LoadFromPointerLIR::makeLoadFromPointer(context,
                        library::kStackPointerVReg, 0).toBase(), instructions);

                // Set the Function context to the current context pointer.
                // TODO: Add the pointer flagging
                linearFrame.append(context, library::HIRId(), library::StoreToPointerLIR::makeStoreToPointer(context,
                        functionVReg, offsetof(schema::FunctionSchema, context), library::kContextPointerVReg).toBase(),
                        instructions);

                // Load the functiondef/method into a register.
                auto functionDefVReg = linearFrame.append(context, library::HIRId(),
                        library::LoadConstantLIR::makeLoadConstant(context,
                        blockLiteralHIR.functionDef().slot()).toBase(), instructions);
                linearFrame.append(context, library::HIRId(), library::StoreToPointerLIR::makeStoreToPointer(context,
                        functionVReg, offsetof(schema::FunctionSchema, def), functionDefVReg).toBase(), instructions);
            } break;

            case library::BranchHIR::nameHash(): {
                auto branchHIR = library::BranchHIR(hir.slot());
                linearFrame.append(context, library::HIRId(), library::BranchLIR::makeBranch(context,
                        branchHIR.blockId()).toBase(), instructions);
            } break;

            case library::BranchIfTrueHIR::nameHash(): {
                auto branchIfTrueHIR = library::BranchIfTrueHIR(hir.slot());
                linearFrame.append(context, library::HIRId(), library::BranchIfTrueLIR::makeBranchIfTrue(context,
                        linearFrame.hirToReg(branchIfTrueHIR.condition()), branchIfTrueHIR.blockId()).toBase(),
                        instructions);
            } break;

            case library::ConstantHIR::nameHash(): {
                auto constantHIR = library::ConstantHIR(hir.slot());
                linearFrame.append(context, constantHIR.id(), library::LoadConstantLIR::makeLoadConstant(context,
                        constantHIR.constant()).toBase(), instructions);
            } break;

            case library::LoadOuterFrameHIR::nameHash(): {
                auto loadOuterFrameHIR = library::LoadOuterFrameHIR(hir.slot());
                auto innerContextVReg = loadOuterFrameHIR.innerContext() ? linearFrame.hirToReg(
                        loadOuterFrameHIR.innerContext()) : library::kFramePointerVReg;
                linearFrame.append(context, loadOuterFrameHIR.id(),
                        library::LoadFromPointerLIR::makeLoadFromPointer(context, innerContextVReg,
                        offsetof(schema::FramePrivateSchema, context)).toBase(), instructions);
            } break;

            case library::MessageHIR::nameHash(): {
                auto messageHIR = library::MessageHIR(hir.slot());
                // Save selector to stack.
                auto selectorVReg = linearFrame.append(context, library::HIRId(),
                        library::LoadConstantLIR::makeLoadConstant(context,
                        messageHIR.selector(context).slot()).toBase(), instructions);
                linearFrame.append(context, library::HIRId(), library::StoreToPointerLIR::makeStoreToPointer(context,
                        library::kStackPointerVReg, 0, selectorVReg).toBase(), instructions);

                // Save number of arguments to stack.
                auto numberOfArgsVReg = linearFrame.append(context, library::HIRId(),
                        library::LoadConstantLIR::makeLoadConstant(context,
                        Slot::makeInt32(messageHIR.arguments().size())).toBase(), instructions);
                linearFrame.append(context, library::HIRId(), library::StoreToPointerLIR::makeStoreToPointer(context,
                        library::kStackPointerVReg, kSlotSize, numberOfArgsVReg).toBase(), instructions);

                // Save number of keyword arguments to stack.
                auto numberOfKeyArgsVReg = linearFrame.append(context, library::HIRId(),
                        library::LoadConstantLIR::makeLoadConstant(context,
                        Slot::makeInt32(messageHIR.keywordArguments().size())).toBase(), instructions);
                linearFrame.append(context, library::HIRId(), library::StoreToPointerLIR::makeStoreToPointer(context,
                        library::kStackPointerVReg, 2 * kSlotSize, numberOfKeyArgsVReg).toBase(), instructions);

                // Save arguments to stack.
                int32_t offset = 3 * kSlotSize;
                for (int32_t i = 0; i < messageHIR.arguments().size(); ++i) {
                    auto argVReg = linearFrame.hirToReg(messageHIR.arguments().typedAt(i));
                    assert(argVReg);
                    linearFrame.append(context, library::HIRId(),
                            library::StoreToPointerLIR::makeStoreToPointer(context, library::kStackPointerVReg, offset,
                            argVReg).toBase(), instructions);
                    offset += kSlotSize;
                }

                // Save keyword arguments to stack.
                for (int32_t i = 0; i < messageHIR.keywordArguments().size(); ++i) {
                    auto argVReg = linearFrame.hirToReg(messageHIR.keywordArguments().typedAt(i));
                    assert(argVReg);
                    linearFrame.append(context, library::HIRId(),
                            library::StoreToPointerLIR::makeStoreToPointer(context, library::kStackPointerVReg, offset,
                            argVReg).toBase(), instructions);
                    offset += kSlotSize;
                }

                // Raise interrupt for a dispatch request.
                linearFrame.append(context, library::HIRId(), library::InterruptLIR::makeInterrupt(context,
                        ThreadContext::InterruptCode::kDispatch).toBase(), instructions);

                // Load return value.
                linearFrame.append(context, messageHIR.id(), library::LoadFromPointerLIR::makeLoadFromPointer(context,
                        library::kStackPointerVReg, 0).toBase(), instructions);
            } break;

            case library::MethodReturnHIR::nameHash(): {
                // Raise return interrupt.
                linearFrame.append(context, library::HIRId(), library::InterruptLIR::makeInterrupt(context,
                        ThreadContext::InterruptCode::kReturn).toBase(), instructions);
            } break;

            case library::PhiHIR::nameHash(): {
                // Shouldn't encounter phis in block statements.
                assert(false);
            } break;

            case library::ReadFromClassHIR::nameHash(): {
                auto readFromClassHIR = library::ReadFromClassHIR(hir.slot());
                auto classVarVReg = linearFrame.hirToReg(readFromClassHIR.classVariableArray());
                linearFrame.append(context, readFromClassHIR.id(),
                        library::LoadFromPointerLIR::makeLoadFromPointer(context, classVarVReg,
                        readFromClassHIR.arrayIndex() * kSlotSize).toBase(), instructions);
            } break;

            case library::ReadFromContextHIR::nameHash(): {
                auto readFromContextHIR = library::ReadFromContextHIR(hir.slot());
                linearFrame.append(context, readFromContextHIR.id(),
                        library::LoadFromPointerLIR::makeLoadFromPointer(context, library::kContextPointerVReg,
                        readFromContextHIR.offset() * kSlotSize).toBase(), instructions);
            } break;

            case library::ReadFromFrameHIR::nameHash(): {
                auto readFromFrameHIR = library::ReadFromFrameHIR(hir.slot());
                auto frameVReg = readFromFrameHIR.frameId() ? linearFrame.hirToReg(readFromFrameHIR.frameId()) :
                        library::kFramePointerVReg;
                linearFrame.append(context, readFromFrameHIR.id(), library::LoadFromPointerLIR::makeLoadFromPointer(
                        context, frameVReg, readFromFrameHIR.frameIndex() * kSlotSize).toBase(), instructions);
            } break;

            case library::ReadFromThisHIR::nameHash(): {
                auto readFromThisHIR = library::ReadFromThisHIR(hir.slot());
                auto thisVReg = linearFrame.hirToReg(readFromThisHIR.thisId());
                linearFrame.append(context, readFromThisHIR.id(), library::LoadFromPointerLIR::makeLoadFromPointer(
                        context, thisVReg, readFromThisHIR.index() * kSlotSize).toBase(), instructions);
            } break;

            case library::RouteToSuperclassHIR::nameHash(): {
                // TODO: super routing, perhaps with a different interrupt code to dispatch?
            } break;

            case library::StoreReturnHIR::nameHash(): {
                // If we're thunking back to C++ code to return we can just put the return value on the stack for now.
                // It needs a special spot, however, to ensure it doesn't get overwritten.
                auto storeReturnHIR = library::StoreReturnHIR(hir.slot());
                auto returnVReg = linearFrame.hirToReg(storeReturnHIR.returnValue());
                linearFrame.append(context, library::HIRId(), library::StoreToPointerLIR::makeStoreToPointer(context,
                        library::kStackPointerVReg, 0, returnVReg).toBase(), instructions);
            } break;

            case library::WriteToClassHIR::nameHash(): {
                auto writeToClassHIR = library::WriteToClassHIR(hir.slot());
                auto classVarVReg = linearFrame.hirToReg(writeToClassHIR.classVariableArray());
                auto toWriteVReg = linearFrame.hirToReg(writeToClassHIR.toWrite());
                linearFrame.append(context, library::HIRId(), library::StoreToPointerLIR::makeStoreToPointer(context,
                        classVarVReg, writeToClassHIR.arrayIndex() * kSlotSize, toWriteVReg).toBase(),
                        instructions);
            } break;

            case library::WriteToFrameHIR::nameHash(): {
                auto writeToFrameHIR = library::WriteToFrameHIR(hir.slot());
                auto frameVReg = writeToFrameHIR.frameId() ? linearFrame.hirToReg(writeToFrameHIR.frameId()) :
                        library::kFramePointerVReg;
                auto toWriteVReg = linearFrame.hirToReg(writeToFrameHIR.toWrite());
                linearFrame.append(context, library::HIRId(), library::StoreToPointerLIR::makeStoreToPointer(context,
                        frameVReg, writeToFrameHIR.frameIndex() * kSlotSize, toWriteVReg).toBase(), instructions);
            } break;

            case library::WriteToThisHIR::nameHash(): {
                auto writeToThisHIR = library::WriteToThisHIR(hir.slot());
                auto thisVReg = linearFrame.hirToReg(writeToThisHIR.thisId());
                auto toWriteVReg = linearFrame.hirToReg(writeToThisHIR.toWrite());
                linearFrame.append(context, library::HIRId(), library::StoreToPointerLIR::makeStoreToPointer(context,
                        thisVReg, writeToThisHIR.index() * kSlotSize, toWriteVReg).toBase(), instructions);
            } break;

            default:
                assert(false); // missing case for hir
            }
        }
    }

    linearFrame.setInstructions(instructions);
    return linearFrame;
}

void BlockSerializer::orderBlocks(ThreadContext* context, library::CFGBlock block,
        std::vector<library::CFGBlock>& blocks, library::TypedArray<library::LabelId> blockOrder) {
    // Mark block as visited by updating number to pointer map.
    blocks[block.id()] = block;
    for (int32_t i = 0; i < block.successors().size(); ++i) {
        auto succ = block.successors().typedAt(i);
        if (!blocks[succ.id()]) {
            orderBlocks(context, succ, blocks, blockOrder);
        }
    }
    blockOrder = blockOrder.typedAdd(context, static_cast<library::LabelId>(block.id()));
}

} // namespace hadron