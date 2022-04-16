#ifndef SRC_HADRON_LIR_STORE_TO_POINTER_LIR_HPP_
#define SRC_HADRON_LIR_STORE_TO_POINTER_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct StoreToPointerLIR : public LIR {
    StoreToPointerLIR() = delete;
    // *(p + off) = store;
    StoreToPointerLIR(VReg p, VReg store, int32_t off):
        LIR(kStoreToPointer, TypeFlags::kNoFlags),
        pointer(p),
        toStore(store),
        offset(off) {
        read(pointer);
        read(toStore);
    }
    virtual ~StoreToPointerLIR() = default;

    VReg pointer;
    VReg toStore;
    int32_t offset;

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
        jit->stxi_w(offset * kSlotSize, locate(pointer), locate(toStore));
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_STORE_TO_POINTER_LIR_HPP_