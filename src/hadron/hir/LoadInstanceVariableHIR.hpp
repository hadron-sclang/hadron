#ifndef SRC_HADRON_HIR_LOAD_INSTANCE_VARIABLE_HIR_HPP_
#define SRC_HADRON_HIR_LOAD_INSTANCE_VARIABLE_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct LoadInstanceVariableHIR : public HIR {
    LoadInstanceVariableHIR() = delete;
    // Need a reference to the instance pointer.
    LoadInstanceVariableHIR(NVID instance, int index, library::Symbol variableName);
    virtual ~LoadInstanceVariableHIR() = default;

    // Need to load the |this| pointer to deference an instance variable.
    NVID instanceID;
    int variableIndex;

    NVID proposeValue(NVID id) override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_LOAD_INSTANCE_VARIABLE_HIR_HPP_