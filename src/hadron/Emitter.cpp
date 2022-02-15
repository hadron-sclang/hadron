#include "hadron/Emitter.hpp"

#include "hadron/JIT.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/lir/LabelLIR.hpp"
#include "hadron/lir/LIR.hpp"

#include "spdlog/spdlog.h"

#include <unordered_map>
#include <vector>

namespace hadron {

void Emitter::emit(LinearFrame* linearFrame, JIT* jit) {
    // Key is block number, value is known addresses of labels, built as they are encountered.
    std::unordered_map<lir::LabelID, JIT::Address> labelAddresses;
    // For forward jumps we record the Label returned and the block number they are targeted to, and patch them all at
    // the end when all the addresses are known.
    std::vector<std::pair<JIT::Label, lir::LabelID>> patchNeeded;

    for (size_t line = 0; line < linearFrame->lineNumbers.size(); ++line) {
        const lir::LIR* lir = linearFrame->lineNumbers[line];

        // Labels need to capture their address before any move predicates so we handle them separately here.
        if (lir->opcode == lir::Opcode::kLabel) {
            const auto label = reinterpret_cast<const lir::LabelLIR*>(lir);
            labelAddresses.emplace(std::make_pair(label->id, jit->address()));
        }

        lir->emit(jit, patchNeeded);
    }

    // Update any patches needed for forward jumps.
    for (auto& patch : patchNeeded) {
        assert(labelAddresses.find(patch.second) != labelAddresses.end());
        jit->patchThere(patch.first, labelAddresses.at(patch.second));
    }
}

} // namespace hadron