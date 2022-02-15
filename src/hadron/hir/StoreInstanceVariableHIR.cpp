#include "hadron/hir/StoreInstanceVariableHIR.hpp"

namespace hadron {
namespace hir {

StoreInstanceVariableHIR::StoreInstanceVariableHIR(NVID instance, NVID storeVal, int index):
    HIR(kStoreInstanceVariable),
    instanceID(instance),
    storeValue(storeVal),
    variableIndex(index) { reads.emplace(storeValue); }

NVID StoreInstanceVariableHIR::proposeValue(NVID /* id */) {
    return kInvalidNVID;
}

} // namespace hir
} // namespace hadron