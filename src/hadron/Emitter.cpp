#include "hadron/Emitter.hpp"

#include "hadron/JIT.hpp"
#include "hadron/library/HadronLIR.hpp"
#include "hadron/MoveScheduler.hpp"
#include "hadron/ThreadContext.hpp"

#include "spdlog/spdlog.h"

#include <unordered_map>
#include <vector>

namespace {

hadron::JIT::Reg locate(hadron::library::LIR lir, hadron::library::VReg vReg) {
    if (vReg.int32() >= 0) {
        auto loc = lir.locations().typedGet(vReg);
        return hadron::JIT::Reg(loc.int32());
    }

    if (vReg == hadron::library::kStackPointerVReg) {
        return hadron::JIT::kStackPointerReg;
    }
    if (vReg == hadron::library::kContextPointerVReg) {
        return hadron::JIT::kContextPointerReg;
    }
    assert(vReg == hadron::library::kFramePointerVReg);
    return hadron::JIT::kFramePointerReg;
}

} // namespace

namespace hadron {

void Emitter::emit(ThreadContext* /* context */, library::LinearFrame linearFrame, JIT* jit) {
    // Key is block number, value is known addresses of labels, built as they are encountered.
    std::unordered_map<int32_t, JIT::Address> labelAddresses;
    // For forward jumps we record the Label returned and the block number they are targeted to, and patch them all at
    // the end when all the addresses are known.
    std::vector<std::pair<JIT::Label, int32_t>> patchNeeded;

    for (int32_t line = 0; line < linearFrame.instructions().size(); ++line) {
        library::LIR lir = linearFrame.instructions().typedAt(line);

        // Labels need to capture their address before any move predicates so we handle them separately here.
        if (lir.className() == library::LabelLIR::nameHash()) {
            const auto label = library::LabelLIR(lir.slot());
            labelAddresses.emplace(std::make_pair(label.labelId().int32(), jit->address()));
        }

        if (lir.moves().size()) {
            MoveScheduler moveScheduler;
            moveScheduler.scheduleMoves(lir.moves(), jit);
        }

        switch (lir.className()) {
        case library::AssignLIR::nameHash(): {
            auto assignLIR = library::AssignLIR(lir.slot());
            jit->movr(locate(lir, assignLIR.vReg()), locate(lir, assignLIR.origin()));
        } break;

        case library::BranchIfTrueLIR::nameHash(): {
            auto branchIfTrueLIR = library::BranchIfTrueLIR(lir.slot());
            auto jitLabel = jit->beqi(locate(lir, branchIfTrueLIR.condition()), 1);
            patchNeeded.emplace_back(std::make_pair(jitLabel, branchIfTrueLIR.labelId().int32()));
        } break;

        case library::BranchLIR::nameHash(): {
            auto branchLIR = library::BranchLIR(lir.slot());
            auto jitLabel = jit->jmp();
            patchNeeded.emplace_back(std::make_pair(jitLabel, branchLIR.labelId().int32()));
        } break;

        case library::BranchToRegisterLIR::nameHash(): {
            auto branchToRegisterLIR = library::BranchToRegisterLIR(lir.slot());
            jit->jmpr(locate(lir, branchToRegisterLIR.address()));
        } break;

        case library::InterruptLIR::nameHash(): {
            auto interruptLIR = library::InterruptLIR(lir.slot());
            // Because all registers have been preserved, we can use hard-coded registers and clobber their values.
            // Save the interrupt code to the threadContext.
            jit->movi(0, interruptLIR.interruptCode().int32());
            jit->stxi_i(offsetof(ThreadContext, interruptCode), JIT::kContextPointerReg, 0);

            // Jump to the exitMachineCode address stored in the threadContext.
            jit->ldxi_w(0, JIT::kContextPointerReg, offsetof(ThreadContext, exitMachineCode));
            jit->jmpr(0);
        } break;

        case library::LabelLIR::nameHash(): {
            // Labels are no-ops.
        } break;

        case library::LoadConstantLIR::nameHash(): {
            auto loadConstantLIR = library::LoadConstantLIR(lir.slot());
            jit->movi(locate(lir, loadConstantLIR.vReg()), loadConstantLIR.constant().asBits());
        } break;

        case library::LoadFromPointerLIR::nameHash(): {
            auto loadFromPointerLIR = library::LoadFromPointerLIR(lir.slot());
            jit->ldxi_w(locate(lir, loadFromPointerLIR.vReg()), locate(lir, loadFromPointerLIR.pointer()),
                    loadFromPointerLIR.offset().int32());
        } break;

        case library::PhiLIR::nameHash(): {
            assert(false);
        } break;

        case library::StoreToPointerLIR::nameHash(): {
            auto storeToPointerLIR = library::StoreToPointerLIR(lir.slot());
            jit->stxi_w(storeToPointerLIR.offset().int32(), locate(lir, storeToPointerLIR.pointer()),
                    locate(lir, storeToPointerLIR.toStore()));
        }

        default:
            assert(false); // missing LIR case for bytecode emission.
        }
    }

    // Update any patches needed for forward jumps.
    for (auto& patch : patchNeeded) {
        assert(labelAddresses.find(patch.second) != labelAddresses.end());
        jit->patchThere(patch.first, labelAddresses.at(patch.second));
    }
}

} // namespace hadron