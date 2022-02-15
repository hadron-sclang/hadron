#ifndef SRC_HADRON_LIR_STORE_TO_STACK_LIR_HPP_
#define SRC_HADRON_LIR_STORE_TO_STACK_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct StoreToStackLIR : public LIR {
    StoreToStackLIR() = delete;
    StoreToStackLIR(VReg store, bool useFP, int32_t off):
        LIR(kStoreToStack, kInvalidVReg, Type::kNone),
        toStore(store),
        useFramePointer(useFP),
        offset(off) {}
    virtual ~StoreToStackLIR() = default;

    VReg toStore;
    bool useFramePointer;
    int32_t offset;

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
        jit->stxi_w(offset, useFramePointer ? JIT::kFramePointerReg : JIT::kStackPointerReg, offset * kSlotSize);
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_STORE_TO_STACK_LIR_HPP_