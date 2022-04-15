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
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_INTERRUPT_LIR_HPP_