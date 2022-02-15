#ifndef SRC_HADRON_HIR_STORE_INSTANCE_VARIABLE_HIR_HPP_
#define SRC_HADRON_HIR_STORE_INSTANCE_VARIABLE_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct StoreInstanceVariableHIR : public HIR {
    StoreInstanceVariableHIR() = delete;
    StoreInstanceVariableHIR(NVID instance, NVID storeVal, int index);
    virtual ~StoreInstanceVariableHIR() = default;

    NVID instanceID;
    NVID storeValue;
    int variableIndex;

    NVID proposeValue(NVID id) override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_STORE_INSTANCE_VARIABLE_HIR_HPP_