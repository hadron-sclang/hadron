#include "hadron/Emitter.hpp"

#include "hadron/BlockBuilder.hpp"
#include "hadron/HIR.hpp"
#include "hadron/JIT.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/MoveScheduler.hpp"
#include "hadron/Slot.hpp"
#include "hadron/ThreadContext.hpp"

#include "spdlog/spdlog.h"

#include <cassert>
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
            jit->ldxi_l(loadArgument->valueLocations.at(loadArgument->value.number), JIT::kStackPointerReg,
                    loadArgument->index * kSlotSize);
        } break;

        case hir::Opcode::kLoadArgumentType: {
            const auto loadArgumentType = reinterpret_cast<const hir::LoadArgumentTypeHIR*>(hir);
            jit->ldxi_l(loadArgumentType->valueLocations.at(loadArgumentType->value.number), JIT::kStackPointerReg,
                    loadArgumentType->index * kSlotSize);
        } break;

        case hir::Opcode::kConstant: {
            // Presence of a ConstantHIR this late in compilation must mean the constant value is needed in the
            // allocated register, so transfer it there.
            const auto constant = reinterpret_cast<const hir::ConstantHIR*>(hir);
            // Note the assumption this is an integer constant.
            assert(constant->constant.getType() == Type::kInteger);
            jit->movi(constant->valueLocations.at(constant->value.number), constant->constant.getInt32());
        } break;

        case hir::Opcode::kStoreReturn: {
            const auto storeReturn = reinterpret_cast<const hir::StoreReturnHIR*>(hir);
            // Add pointer tag to stack pointer to maintain invariant that saved pointers are always tagged.
            jit->ori(JIT::kStackPointerReg, JIT::kStackPointerReg, Slot::kObjectTag);
            // Save the stack pointer to the thread context so we can load the frame pointer in its place.
            jit->stxi_w(offsetof(ThreadContext, stackPointer), JIT::kContextPointerReg, JIT::kStackPointerReg);
            // Load and untag Frame Pointer
            jit->ldxi_w(JIT::kStackPointerReg, JIT::kContextPointerReg, offsetof(ThreadContext, framePointer));
            jit->andi(JIT::kStackPointerReg, JIT::kStackPointerReg, ~Slot::kTagMask);
            // NOTE ASSUMPTION OF INTEGER TYPE
            auto valReg = storeReturn->valueLocations.at(storeReturn->returnValue.first.number);
            jit->andi(valReg, valReg, ~Slot::kTagMask);
            jit->ori(valReg, valReg, Slot::kInt32Tag);
            // Now store the tagged result value and type at the frame pointer location.
            jit->str_l(JIT::kStackPointerReg, valReg);
            // Now restore the stack pointer.
            jit->ldxi_w(JIT::kStackPointerReg, JIT::kContextPointerReg, offsetof(ThreadContext, stackPointer));
            jit->andi(JIT::kStackPointerReg, JIT::kStackPointerReg, ~Slot::kTagMask);
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

    // TODO: patches?

    // Load caller return address from framePointer + 1 into the stack pointer, then jump there.
    jit->ldxi_w(JIT::kStackPointerReg, JIT::kContextPointerReg, offsetof(ThreadContext, framePointer));
    jit->andi(JIT::kStackPointerReg, JIT::kStackPointerReg, ~Slot::kTagMask);
    jit->ldxi_w(JIT::kStackPointerReg, JIT::kStackPointerReg, kSlotSize);
    jit->andi(JIT::kStackPointerReg, JIT::kStackPointerReg, ~Slot::kTagMask);
    jit->jmpr(JIT::kStackPointerReg);
}

} // namespace hadron