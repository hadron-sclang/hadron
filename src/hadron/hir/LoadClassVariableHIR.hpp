#ifndef SRC_HADRON_HIR_LOAD_CLASS_VARIABLE_HPP_
#define SRC_HADRON_HIR_LOAD_CLASS_VARIABLE_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct LoadClassVariableHIR : public HIR {
    LoadClassVariableHIR() = delete;
    LoadClassVariableHIR(int index, library::Symbol name);
    virtual ~LoadClassVariableHIR() = default;

    int32_t variableIndex;

    NVID proposeValue(NVID id) override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_LOAD_CLASS_VARIABLE_HPP_