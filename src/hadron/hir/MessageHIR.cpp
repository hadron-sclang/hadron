#include "hadron/hir/MessageHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/InterruptLIR.hpp"
#include "hadron/lir/LoadConstantLIR.hpp"
#include "hadron/lir/LoadFromPointerLIR.hpp"
#include "hadron/lir/StoreToPointerLIR.hpp"

namespace hadron {
namespace hir {

MessageHIR::MessageHIR(): HIR(kMessage, TypeFlags::kAllFlags) {}

void MessageHIR::addArgument(ID id) {
    reads.emplace(id);
    arguments.emplace_back(id);
}

void MessageHIR::addKeywordArgument(ID id) {
    reads.emplace(id);
    keywordArguments.emplace_back(id);
}

ID MessageHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool MessageHIR::replaceInput(ID original, ID replacement) {
    if (!replaceReads(original, replacement)) { return false; }

    for (size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i] == original) {
            arguments[i] = replacement;
        }
    }

    for (size_t i = 0; i < keywordArguments.size(); ++i) {
        if (keywordArguments[i] == original) {
            keywordArguments[i] = replacement;
        }
    }

    return true;
}

void MessageHIR::lower(LinearFrame* linearFrame) const {
    auto selectorVReg = linearFrame->append(hir::kInvalidID, std::make_unique<lir::LoadConstantLIR>(selector.slot()));
    linearFrame->append(hir::kInvalidID, std::make_unique<lir::StoreToPointerLIR>(lir::kStackPointerVReg, selectorVReg,
            0)); // TODO - Stack Structure?

    auto numberOfArgsVReg = linearFrame->append(hir::kInvalidID, std::make_unique<lir::LoadConstantLIR>(
            Slot::makeInt32(arguments.size())));
    linearFrame->append(hir::kInvalidID, std::make_unique<lir::StoreToPointerLIR>(lir::kStackPointerVReg,
            numberOfArgsVReg, 1));

    auto numberOfKeyArgsVReg = linearFrame->append(hir::kInvalidID, std::make_unique<lir::LoadConstantLIR>(
            Slot::makeInt32(keywordArguments.size())));
    linearFrame->append(hir::kInvalidID, std::make_unique<lir::StoreToPointerLIR>(lir::kStackPointerVReg,
            numberOfKeyArgsVReg, 2));

    int32_t index = 3;
    for (auto id : arguments) {
        auto vReg = linearFrame->hirToReg(id);
        linearFrame->append(hir::kInvalidID, std::make_unique<lir::StoreToPointerLIR>(lir::kStackPointerVReg,
                vReg, index));
        ++index;
    }

    for (auto id : keywordArguments) {
        auto vReg = linearFrame->hirToReg(id);
        linearFrame->append(hir::kInvalidID, std::make_unique<lir::StoreToPointerLIR>(lir::kStackPointerVReg,
                vReg, index));
        ++index;
    }

    linearFrame->append(hir::kInvalidID, std::make_unique<lir::InterruptLIR>(ThreadContext::InterruptCode::kDispatch));

    // Load return value.
    linearFrame->append(id, std::make_unique<lir::LoadFromPointerLIR>(lir::kStackPointerVReg, 3));
}

} // namespace hir
} // namespace hadron
