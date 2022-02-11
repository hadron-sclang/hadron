#ifndef SRC_HADRON_LIR_STORE_TO_POINTER_LIR_HPP_
#define SRC_HADRON_LIR_STORE_TO_POINTER_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct StoreToPointerLIR : public LIR {
    StoreToPointerLIR() = delete;
    // *(p + off) = store;
    StoreToPointerLIR(VReg p, VReg store, int32_t off):
        LIR(kStoreToPointer, kInvalidVReg, Type::kNone),
        pointer(p),
        toStore(store),
        offset(off) {}
    virtual ~StoreToPointerLIR() = default;

    VReg pointer;
    VReg toStore;
    int32_t offset;
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_STORE_TO_POINTER_LIR_HPP_