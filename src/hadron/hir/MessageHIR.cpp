#include "hadron/hir/MessageHIR.hpp"

namespace hadron {
namespace hir {

MessageHIR::MessageHIR():
        HIR(kMessage, Type::kAny, library::Symbol()),
        selector(kInvalidNVID) {}

NVID MessageHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

} // namespace hir
} // namespace hadron
