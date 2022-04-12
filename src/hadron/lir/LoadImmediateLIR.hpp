#ifndef SRC_HADRON_LIR_LOAD_IMMEDIATE_LIR_HPP_
#define SRC_HADRON_LIR_LOAD_IMMEDIATE_LIR_HPP_

namespace hadron {
namespace lir {

struct LoadImmediateLIR : public LIR {
    LoadImmediateLIR() = delete;
    explicit LoadImmediateLIR(void* p):
        LIR(kLoadImmediate, TypeFlags::kAllFlags),
        pointer(p) {}
    virtual ~LoadImmediateLIR() = default;

    void* pointer;

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
        jit->ldi_l(locations.at(value), pointer);
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_LOAD_IMMEDIATE_LIR_HPP_