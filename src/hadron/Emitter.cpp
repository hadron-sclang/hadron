#include "hadron/Emitter.hpp"

#include "hadron/BlockBuilder.hpp"
#include "hadron/JIT.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/lir/LabelLIR.hpp"
#include "hadron/lir/LIR.hpp"
#include "hadron/Slot.hpp"
#include "hadron/ThreadContext.hpp"

#include "spdlog/spdlog.h"

#include <cassert>
#include <unordered_map>
#include <vector>

namespace hadron {

void Emitter::emit(LinearBlock* linearBlock, JIT* jit) {
    // Key is block number, value is known addresses of labels, built as they are encountered.
    std::unordered_map<int, JIT::Address> labelAddresses;
    // For forward jumps we record the Label returned and the block number they are targeted to, and patch them all at
    // the end when all the addresses are known.
    std::vector<std::pair<JIT::Label, int>> jmpPatchNeeded;

    for (size_t line = 0; line < linearBlock->lineNumbers.size(); ++line) {
        SPDLOG_DEBUG("Emitting line {}", line);

        const lir::LIR* lir = linearBlock->lineNumbers[line];

        // Labels need to capture their address before any move predicates so we handle them separately here.
        if (lir->opcode == lir::Opcode::kLabel) {
            const auto label = reinterpret_cast<const lir::LabelLIR*>(lir);
            JIT::Address address = jit->address();
            labelAddresses.emplace(std::make_pair(label->blockNumber, address));
        }

        lir->emit(jit);

/*

        // Emit any predicate moves if required.
        if (lir->moves.size()) {
            moveScheduler.scheduleMoves(lir->moves, jit);
        }

        switch(lir->opcode) {
        case lir::Opcode::kLoadArgument: {
            const auto loadArgument = reinterpret_cast<const lir::LoadArgumentLIR*>(lir);
            jit->ldxi_l(loadArgument->valueLocations.at(loadArgument->value.number), JIT::kStackPointerReg,
                    loadArgument->index * kSlotSize);
        } break;

        case lir::Opcode::kLoadArgumentType: {
            const auto loadArgumentType = reinterpret_cast<const lir::LoadArgumentTypeLIR*>(lir);
            jit->ldxi_l(loadArgumentType->valueLocations.at(loadArgumentType->value.number), JIT::kStackPointerReg,
                    loadArgumentType->index * kSlotSize);
        } break;

        case lir::Opcode::kConstant: {
            // Presence of a ConstantLIR this late in compilation must mean the constant value is needed in the
            // allocated register, so transfer it there.
            const auto constant = reinterpret_cast<const lir::ConstantLIR*>(lir);
            switch (constant->constant.getType()) {
            case Type::kNil:
                jit->movi(constant->valueLocations.at(constant->value.number), Slot::kNilTag);
                break;
            case Type::kInteger:
                jit->movi(constant->valueLocations.at(constant->value.number), constant->constant.getInt32());
                break;
            case Type::kBoolean:
                jit->movi(constant->valueLocations.at(constant->value.number), constant->constant.getBool() ? 1 : 0);
                break;
            case Type::kSymbol:
                jit->movi(constant->valueLocations.at(constant->value.number), constant->constant.getHash());
                break;
            default:
                assert(false);
                break;
            }
        } break;

        case lir::Opcode::kLoadInstanceVariable:
        case lir::Opcode::kLoadInstanceVariableType:
        case lir::Opcode::kLoadClassVariable:
        case lir::Opcode::kLoadClassVariableType:
        case lir::Opcode::kStoreInstanceVariable:
        case lir::Opcode::kStoreClassVariable:
            assert(false); // TODO
            break;

        case lir::Opcode::kMethodReturn: {
            const auto storeReturn = reinterpret_cast<const lir::MethodReturnLIR*>(lir);
            // Add pointer tag to stack pointer to maintain invariant that saved pointers are always tagged.
            jit->ori(JIT::kStackPointerReg, JIT::kStackPointerReg, Slot::kPointerTag);
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

        case lir::Opcode::kPhi:
            // Should not be encountered inline in resolved code.
            assert(false);
            break;

        case lir::Opcode::kBranch: {
            const auto branch = reinterpret_cast<const lir::BranchLIR*>(lir);
            auto targetRange = linearBlock->blockRanges[branch->blockNumber];
            // If the block is before line this is a backwards jump, the address is already known, jump immediately
            // there.
            if (line > targetRange.first) {
                assert(labelAddresses.find(branch->blockNumber) != labelAddresses.end());
                jit->jmpi(labelAddresses.at(branch->blockNumber));
            } else if (line + 1 < targetRange.first) {
                // If the branch is to the first instruction in the next consecutive block, and this branch is the
                // instruction directly before that block, we can omit the branch. So we only take action if the forward
                // jump is for greater than the next LIR instruction.
                jmpPatchNeeded.emplace_back(std::make_pair(jit->jmp(), branch->blockNumber));
            }
        } break;

        case lir::Opcode::kBranchIfTrue: {
            const auto branchIfTrue = reinterpret_cast<const lir::BranchIfTrueLIR*>(lir);
            // document assumption this is always a forward jump.
            assert(line + 1 < linearBlock->blockRanges[branchIfTrue->blockNumber].first);
            jmpPatchNeeded.emplace_back(std::make_pair(
                    jit->beqi(branchIfTrue->valueLocations.at(branchIfTrue->condition.first.number), 1),
                    branchIfTrue->blockNumber));
        } break;

        case lir::Opcode::kLabel:
            // Should handle labels before move predicates, making them no-ops here.
            break;

        case lir::Opcode::kDispatchSetupStack: {
        } break;

        case lir::Opcode::kDispatchStoreArg: {
        } break;

        case lir::Opcode::kDispatchStoreKeyArg: {
        } break;

        case lir::Opcode::kDispatchCall: {
        } break;

        case lir::Opcode::kDispatchLoadReturn: {
        } break;

        case lir::Opcode::kDispatchLoadReturnType: {
        } break;

        case lir::Opcode::kDispatchCleanup: {
        } break;
        }
    */
    }

    // Update any patches needed for forward jumps.
    for (auto& patch : jmpPatchNeeded) {
        assert(labelAddresses.find(patch.second) != labelAddresses.end());
        jit->patchThere(patch.first, labelAddresses.at(patch.second));
    }

    // Load caller return address from framePointer + 1 into the stack pointer, then jump there.
    jit->ldxi_w(JIT::kStackPointerReg, JIT::kContextPointerReg, offsetof(ThreadContext, framePointer));
    jit->andi(JIT::kStackPointerReg, JIT::kStackPointerReg, ~Slot::kTagMask);
    jit->ldxi_w(JIT::kStackPointerReg, JIT::kStackPointerReg, kSlotSize);
    jit->andi(JIT::kStackPointerReg, JIT::kStackPointerReg, ~Slot::kTagMask);
    jit->jmpr(JIT::kStackPointerReg);
}

} // namespace hadron