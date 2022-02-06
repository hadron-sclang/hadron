#ifndef SRC_HADRON_HIR_STORE_CLASS_VARIABLE_HIR_HPP_
#define SRC_HADRON_HIR_STORE_CLASS_VARIABLE_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct StoreClassVariableHIR : public HIR {
    StoreClassVariableHIR() = delete;
    StoreClassVariableHIR(NVID storeVal, int index);
    virtual ~StoreClassVariableHIR() = default;

    NVID storeValue;
    int variableIndex;

    NVID proposeValue(NVID id) override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_STORE_CLASS_VARIABLE_HIR_HPP_