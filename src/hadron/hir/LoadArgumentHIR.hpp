#ifndef SRC_HADRON_HIR_LOAD_ARGUMENT_HIR_HPP_
#define SRC_HADRON_HIR_LOAD_ARGUMENT_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

// Loads the argument at |index| from the stack.
struct LoadArgumentHIR : public HIR {
    LoadArgumentHIR() = delete;
    LoadArgumentHIR(int argIndex, library::Symbol name, bool varArgs = false);
    virtual ~LoadArgumentHIR() = default;
    int index;
    bool isVarArgs;

    // Forces the kAny type for all arguments.
    NVID proposeValue(NVID id) override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_LOAD_ARGUMENT_HIR_HPP_