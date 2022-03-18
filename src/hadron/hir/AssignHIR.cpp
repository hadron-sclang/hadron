#include "hadron/hir/AssignHIR.hpp"

#include "hadron/lir/AssignLIR.hpp"

#include <spdlog/spdlog.h>

namespace hadron {
namespace hir {

AssignHIR::AssignHIR(library::Symbol n, ID value):
    HIR(kAssign), name(n), valueId(value) { reads.emplace(valueId); }

ID AssignHIR::proposeValue(ID /* proposedId */) {
    return hir::kInvalidID;
}

bool AssignHIR::replaceInput(ID original, ID replacement) {
    if (replaceReads(original, replacement)) {
        SPDLOG_INFO("AssignHIR replacing {} with {}", original, replacement);
        assert(valueId == original);
        valueId = replacement;
        return true;
    }

    return false;
}

void AssignHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>&  /* vRegs */,
        LIRList& /* append */) const {
    // usually a no-op unless the name is *captured*
    assert(false);
}

} // namespace hir
} // namespace hadron