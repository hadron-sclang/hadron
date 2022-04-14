#include "hadron/hir/BlockLiteralHIR.hpp"

#include "hadron/Block.hpp"
#include "hadron/Frame.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/library/Function.hpp"
#include "hadron/lir/LoadConstantLIR.hpp"
#include "hadron/lir/StoreToPointerLIR.hpp"
#include "hadron/Scope.hpp"

namespace hadron {
namespace hir {

BlockLiteralHIR::BlockLiteralHIR():
    HIR(kBlockLiteral, TypeFlags::kObjectFlag) {}

ID BlockLiteralHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool BlockLiteralHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void BlockLiteralHIR::lower(LinearFrame* linearFrame) const {
    // If this BlockLiteral is being lowered that means that it was not inlined during optimization, and so should be
    // compiled down to bytecode as part of a FunctionDef, then the BlockLiteral becomes a call to create a new
    // Function object. The Materializer should have already compiled the FunctionDef and provided it to the block.
    assert(!functionDef.isNil());

    // Interrupt to allocate memory for the Function object.
    auto functionVReg = linearFrame->append(id, std::make_unique<lir::InterruptLIR>());

    // Set the Function context to the current context pointer. Make a copy of the current context register.
    auto contextVReg = linearFrame->append(kInvalidID, std::make_unique<lir::AssignLIR>(lir::kContextPointerVReg));
    // TODO: Add the pointer flagging
    linearFrame->append(kInvalidID, std::make_unique<lir::StoreToPointerLIR>(functionVReg, contextVReg,
            offsetof(schema::FunctionSchema, context)));

    // Load the functiondef/method into a register.
    auto functionDefVReg = linearFrame->append(kInvalidID, std::make_unique<lir::LoadConstantLIR>(functionDef.slot()));
    linearFrame->append(kInvalidID, std::make_unique<lir::StoreToPointerLIR>(functionVReg, functionDefVReg,
            offsetof(schema::FunctionSchema, def)));
}

} // namespace hir
} // namespace hadron