#include "hadron/hir/BlockLiteralHIR.hpp"

#include "hadron/Block.hpp"
#include "hadron/Frame.hpp"
#include "hadron/hir/ImportLocalVariableHIR.hpp"
#include "hadron/Scope.hpp"

namespace hadron {
namespace hir {

BlockLiteralHIR::BlockLiteralHIR():
    HIR(kBlockLiteral, TypeFlags::kObjectFlag, library::Symbol()) {}

NVID BlockLiteralHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

bool BlockLiteralHIR::replaceInput(NVID original, NVID replacement) {
    assert(frame);
    bool modified = false;

    // The frame could have ImportLocalValueHIR that has |original| as the imported value, so inspect the first block
    // within the frame.
    for (auto& hir : frame->rootScope->blocks.front()->statements) {
        if (hir->opcode == hir::Opcode::kImportLocalVariable) {
            auto* importLocal = reinterpret_cast<ImportLocalVariableHIR*>(hir.get());
            if (importLocal->externalId == original) {
                modified = true;
                importLocal->externalId = replacement;
            }
        }
    }

    return modified;
}

void BlockLiteralHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    // WRITEME
    assert(false);
}

} // namespace hir
} // namespace hadron