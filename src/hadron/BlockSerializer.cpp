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
    auto linearFrame = library::LinearFrame::alloc(context);
    linearFrame.initToNil();
    linearFrame.setBlockLabels(library::TypedArray<library::LabelLIR>::typedNewClear(context,
            frame.numberOfBlocks()));

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

        auto labelPhis = library::TypedArray<library::LIR>::typedArrayAlloc(context, block.phis().size());
        for (int32_t i = 0; i < block.phis().size(); ++i) {
            auto phi = block.phis().typedAt(i);

            auto phiLIR = std::make_unique<lir::PhiLIR>();

            for (auto nvid : inputs) {
                auto vReg = linearFrame->hirToReg(nvid);
                assert(vReg != lir::kInvalidVReg);
                phiLIR->addInput(linearFrame->vRegs[vReg]->get());
            }

            return phiLIR;

            auto phiLIR = phi.lowerPhi(context, linearFrame);
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


            case library::BranchHIR::nameHash():
            case library::BranchIfTrueHIR::nameHash():
            case library::ConstantHIR::nameHash():
            case library::LoadOuterFrameHIR::nameHash():
            case library::MessageHIR::nameHash():
            case library::MethodReturnHIR::nameHash():
            case library::PhiHIR::nameHash():
            case library::ReadFromClassHIR::nameHash():
            case library::ReadFromContextHIR::nameHash():
            case library::ReadFromFrameHIR::nameHash():
            case library::ReadFromThisHIR::nameHash():
            case library::RouteToSuperclassHIR::nameHash():
            case library::StoreReturnHIR::nameHash():
            case library::WriteToClassHIR::nameHash():
            case library::WriteToFrameHIR::nameHash():
            case library::WriteToThisHIR::nameHash():

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