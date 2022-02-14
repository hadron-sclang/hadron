#include "hadron/Emitter.hpp"

#include "hadron/Block.hpp"
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
    std::unordered_map<Block::ID, JIT::Address> labelAddresses;
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
            labelAddresses.emplace(std::make_pair(label->blockId, address));
        }

        lir->emit(jit);
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