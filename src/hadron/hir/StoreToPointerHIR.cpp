#include "hadron/hir/StoreToPointerHIR.hpp"

namespace hadron {
namespace hir {

StoreToPointerHIR::StoreToPointerHIR(void* p, NVID storeVal):
    HIR(kStoreToPointer),
    pointer(p),
    storeValue(storeVal) { reads.emplace(storeValue); }

NVID StoreToPointerHIR::proposeValue(NVID /* id */) {
    return kInvalidNVID;
}

} // namespace hir
} // namespace hadron