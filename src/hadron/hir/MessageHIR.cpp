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

void MessageHIR::lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const {
    // The problem: we want to set up a new stack frame beyond the current register spill area. However we won't know
    // until *after* register allocation how many spill slots we need. Always assuming we need numberOfReg - 2 + 1
    // spill slots seems wasteful, that's potentially 248 bytes of extra space on the stack for every function call.
    // So we need a LIR that is like "EnterNewStackFrame" that produces a frame pointer pointing *just past* the end
    // of the register spill area, and then around like move scheduling time we know how many spill slots we need,
    // we can just fix up the instruction then.
}

} // namespace hir
} // namespace hadron
