#include "hadron/Emitter.hpp"

#include "hadron/BlockBuilder.hpp"
#include "hadron/HIR.hpp"
#include "hadron/JIT.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/MoveScheduler.hpp"
#include "hadron/Slot.hpp"
#include "hadron/ThreadContext.hpp"

#include <unordered_map>
#include <vector>

namespace hadron {

void Emitter::emit(LinearBlock* linearBlock, JIT* jit) {
    MoveScheduler moveScheduler;
    // Key is block number, value is known addresses of labels, built as they are encountered.
    std::unordered_map<int, JIT::Address> labelAddresses;
    // For forward jumps we record the Label returned and patch them all at the end when all the addresses are known.
    std::vector<std::pair<int, JIT::Label>> jmpPatchNeeded;

    for (size_t line = 0; line < linearBlock->instructions.size(); ++line) {
        const hir::HIR* hir = linearBlock->instructions[line].get();

        // Labels need to capture their address before any move predicates so we handle them separately here.
        if (hir->opcode == hir::Opcode::kLabel) {
            const auto label = reinterpret_cast<const hir::LabelHIR*>(hir);
            JIT::Address address = jit->address();
            labelAddresses.emplace(std::make_pair(label->blockNumber, address));
        }

        // Emit any predicate moves if required.
        if (hir->moves.size()) {
            moveScheduler.scheduleMoves(hir->moves, jit);
        }

        switch(hir->opcode) {
        case hir::Opcode::kLoadArgument: {
            const auto loadArgument = reinterpret_cast<const hir::LoadArgumentHIR*>(hir);
            jit->ldxi_w(loadArgument->valueLocations.at(loadArgument->value.number), JIT::kStackPointerReg,
                    Slot::slotValueOffset(loadArgument->index));
        } break;

        case hir::Opcode::kLoadArgumentType: {
            const auto loadArgumentType = reinterpret_cast<const hir::LoadArgumentTypeHIR*>(hir);
            jit->ldxi_w(loadArgumentType->valueLocations.at(loadArgumentType->value.number), JIT::kStackPointerReg,
                    Slot::slotTypeOffset(loadArgumentType->index));
        } break;

        case hir::Opcode::kConstant: {
            // Presence of a ConstantHIR this late in compilation must mean the constant value is needed in the
            // allocated register, so transfer it there.
            const auto constant = reinterpret_cast<const hir::ConstantHIR*>(hir);
            jit->movi(constant->valueLocations.at(constant->value.number), constant->constant.value.intValue);
        } break;

        case hir::Opcode::kStoreReturn: {
            const auto storeReturn = reinterpret_cast<const hir::StoreReturnHIR*>(hir);
            // Save the stack pointer to the thread context so we can load the frame pointer in its place.
            jit->stxi_w(offsetof(ThreadContext, stackPointer), JIT::kContextPointerReg, JIT::kStackPointerReg);
            jit->ldxi_w(JIT::kStackPointerReg, JIT::kContextPointerReg, offsetof(ThreadContext, framePointer));
            // Now store the result value and type at the frame pointer location.
            jit->stxi_w(Slot::slotValueOffset(0), JIT::kStackPointerReg,
                    storeReturn->valueLocations.at(storeReturn->returnValue.first.number));
            jit->stxi_w(Slot::slotTypeOffset(0), JIT::kStackPointerReg,
                    storeReturn->valueLocations.at(storeReturn->returnValue.second.number));
            // Now restore the stack pointer.
            jit->ldxi_w(JIT::kStackPointerReg, JIT::kContextPointerReg, offsetof(ThreadContext, stackPointer));
        } break;

        case hir::Opcode::kResolveType:
            // no-op
            break;

        case hir::Opcode::kPhi:
            // Should not be encountered inline in resolved code.
            assert(false);
            break;

        case hir::Opcode::kBranch: {
            const auto branch = reinterpret_cast<const hir::BranchHIR*>(hir);
            auto targetRange = linearBlock->blockRanges[branch->blockNumber];
            // If the block is before line this is a backwards jump, the address is already known, jump immediately
            // there.
            if (line > targetRange.first) {
                jit->jmpi(labelAddresses.at(branch->blockNumber));
            } else if (line + 1 < targetRange.first) {
                // If the branch is to the first instruction in the next consecutive block, and this branch is the
                // instruction directly before that block, we can omit the branch. So we only take action if the forward
                // jump is for greater than the next HIR instruction.
                jmpPatchNeeded.emplace_back(std::make_pair(branch->blockNumber, jit->jmp()));
            }
        } break;

        case hir::Opcode::kBranchIfZero: {
            const auto branchIfZero = reinterpret_cast<const hir::BranchIfZeroHIR*>(hir);
            jmpPatchNeeded.emplace_back(std::make_pair(branchIfZero->blockNumber,
                    jit->beqi(branchIfZero->valueLocations.at(branchIfZero->condition.first.number), 0)));
        } break;

        case hir::Opcode::kLabel:
            // Should handle labels before move predicates, making them no-ops here.
            break;

        case hir::Opcode::kDispatchCall: {
            // TODO: dispatch code.
        } break;

        case hir::Opcode::kDispatchLoadReturn: {
        } break;

        case hir::Opcode::kDispatchLoadReturnType: {
        } break;

        case hir::Opcode::kDispatchCleanup: {
        } break;
        }
    }
}

} // namespace hadron