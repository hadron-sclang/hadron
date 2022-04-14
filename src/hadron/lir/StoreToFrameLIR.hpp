#ifndef SRC_HADRON_LIR_STORE_TO_FRAME_LIR_HPP_
#define SRC_HADRON_LIR_STORE_TO_FRAME_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct StoreToFrameLIR : public LIR {
    StoreToFrameLIR() = delete;
    StoreToFrameLIR(VReg store, int32_t off):
        LIR(kStoreToFrame, TypeFlags::kNoFlags),
        toStore(store),
        offset(off) {
            reads.emplace(toStore);
    }
    virtual ~StoreToFrameLIR() = default;

    VReg toStore;
    int32_t offset;

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
        jit->stxi_w(offset * kSlotSize, JIT::kFramePointerReg, locations.at(toStore));
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_STORE_TO_FRAME_LIR_HPP_