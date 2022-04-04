#include "hadron/hir/BlockLiteralHIR.hpp"

#include "hadron/Block.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Scope.hpp"

namespace hadron {
namespace hir {

BlockLiteralHIR::BlockLiteralHIR():
    HIR(kBlockLiteral, TypeFlags::kObjectFlag) {}

ID BlockLiteralHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool BlockLiteralHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void BlockLiteralHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    // WRITEME
    assert(false);
}

} // namespace hir
} // namespace hadron