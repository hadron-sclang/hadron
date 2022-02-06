#ifndef SRC_HADRON_HIR_LOAD_INSTANCE_VARIABLE_HIR_HPP_
#define SRC_HADRON_HIR_LOAD_INSTANCE_VARIABLE_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct LoadInstanceVariableHIR : public HIR {
    LoadInstanceVariableHIR() = delete;
    LoadInstanceVariableHIR(std::pair<Value, Value> thisVal, int32_t index);
    virtual ~LoadInstanceVariableHIR() = default;

    // Need to load the |this| pointer to deference an instance variable.
    std::pair<Value, Value> thisValue;
    int32_t variableIndex;

    Value proposeValue(uint32_t number) override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_LOAD_INSTANCE_VARIABLE_HIR_HPP_