#ifndef SRC_HADRON_LIR_LOAD_FROM_POINTER_LIR_HPP_
#define SRC_HADRON_LIR_LOAD_FROM_POINTER_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct LoadFromPointerLIR : public LIR {
    LoadFromPointerLIR() = delete;
    LoadFromPointerLIR(VReg v, VReg p, int32_t off):
        LIR(kLoadFromPointer, v, Type::kAny),
        pointer(p),
        offset(off) {}
    virtual ~LoadFromPointerLIR() = default;

    VReg pointer;
    int32_t offset;
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_LOAD_FROM_POINTER_LIR_HPP_