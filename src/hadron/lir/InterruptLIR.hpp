#ifndef SRC_HADRON_LIR_INTERRUPT_LIR_HPP_
#define SRC_HADRON_LIR_INTERRUPT_LIR_HPP_

#include "hadron/lir/LIR.hpp"
#include "hadron/ThreadContext.hpp"

namespace hadron {
namespace lir {

struct InterruptLIR : public LIR {
    InterruptLIR() = delete;
    explicit InterruptLIR(ThreadContext::InterruptCode code):
        LIR(kInterrupt, TypeFlags::kNoFlags),
        interruptCode(code) {}
    virtual ~InterruptLIR() = default;

    ThreadContext::InterruptCode interruptCode;

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
        // Because all registers have been preserved, we can use hard-coded registers and clobber their values.
        // Save the interrupt code to the threadContext.
        jit->movi(0, interruptCode);
        jit->stxi_i(offsetof(ThreadContext, interruptCode), JIT::kContextPointerReg, 0);

        // Jump to the exitMachineCode address stored in the threadContext.
        jit->ldxi_w(0, JIT::kContextPointerReg, offsetof(ThreadContext, exitMachineCode));
        jit->jmpr(0);
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_INTERRUPT_LIR_HPP_