#include "hadron/hir/ConstantHIR.hpp"

namespace hadron {
namespace hir {

ConstantHIR::ConstantHIR(const Slot c): HIR(kConstant, c.getType(), library::Symbol()), constant(c) {}

ConstantHIR::ConstantHIR(const Slot c, library::Symbol name):
        HIR(kConstant, c.getType(), name), constant(c) {}

NVID ConstantHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

} // namespace hir
} // namespace hadron
