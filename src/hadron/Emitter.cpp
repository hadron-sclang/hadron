#include "hadron/Emitter.hpp"

#include "hadron/JIT.hpp"
#include "hadron/library/HadronLIR.hpp"
#include "hadron/MoveScheduler.hpp"
#include "hadron/ThreadContext.hpp"

#include "spdlog/spdlog.h"

#include <unordered_map>
#include <vector>

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
        case library::AssignLIR::nameHash():
        case library::BranchIfTrueLIR::nameHash():
        case library::BranchLIR::nameHash():
        case library::BranchToRegisterLIR::nameHash():
        case library::InterruptLIR::nameHash():
        case library::LabelLIR::nameHash():
        case library::LoadConstantLIR::nameHash():
        case library::LoadFromPointerLIR::nameHash():
        case library::PhiLIR::nameHash():
        case library::StoreToPointerLIR::nameHash():

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