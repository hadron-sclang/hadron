#include "hadron/hir/BranchHIR.hpp"

namespace hadron {
namespace hir {

BranchHIR::BranchHIR(): HIR(kBranch) {}

NVID BranchHIR::proposeValue(NVID id) {
    return kInvalidNVID;
}

} // namespace hir
} // namespace hadron
