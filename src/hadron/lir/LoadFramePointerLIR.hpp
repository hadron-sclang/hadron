#ifndef SRC_HADRON_LIR_LOAD_FRAME_POINTER_LIR_HPP_
#define SRC_HADRON_LIR_LOAD_FRAME_POINTER_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct LoadFramePointerLIR : public LIR {
    LoadFramePointerLIR() = delete;
    explicit LoadFramePointerLIR(VReg v): LIR(kLoadFramePointer, v, Type::kObject) {}
    virtual ~LoadFramePointerLIR() = default;
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_LOAD_FRAME_POINTER_LIR_HPP_