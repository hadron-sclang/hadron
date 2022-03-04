#include "hadron/hir/BlockLiteralHIR.hpp"

#include "hadron/Block.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Scope.hpp"

namespace hadron {
namespace hir {

BlockLiteralHIR::BlockLiteralHIR():
    HIR(kBlockLiteral, TypeFlags::kObjectFlag, library::Symbol()) {}

NVID BlockLiteralHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

void BlockLiteralHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    // WRITEME
    assert(false);
}

} // namespace hir
} // namespace hadron