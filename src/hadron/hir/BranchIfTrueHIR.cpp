#include "hadron/hir/BranchIfTrueHIR.hpp"

namespace hadron {
namespace hir {

BranchIfTrueHIR::BranchIfTrueHIR(NVID cond): HIR(kBranchIfTrue), condition(cond) {}

NVID BranchIfTrueHIR::proposeValue(NVID /* id */) {
    return kInvalidNVID;
}

} // namespace hir
} // namespace hadron
