#include "hadron/hir/StoreClassVariableHIR.hpp"

namespace hadron {
namespace hir {

StoreClassVariableHIR::StoreClassVariableHIR(NVID storeVal, int index):
    HIR(kStoreClassVariable),
    storeValue(storeVal),
    variableIndex(index) { reads.emplace(storeValue); }

NVID StoreClassVariableHIR::proposeValue(NVID id) {
    return kInvalidNVID;
}

} // namespace hir
} // namespace hadron