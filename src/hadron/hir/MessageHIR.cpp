#include "hadron/hir/MessageHIR.hpp"

#include "hadron/lir/LoadConstantLIR.hpp"
#include "hadron/Signature.hpp"

namespace hadron {
namespace hir {

MessageHIR::MessageHIR(): HIR(kMessage, TypeFlags::kAllFlags, library::Symbol()) {}

std::unique_ptr<Signature> MessageHIR::signature(const std::vector<HIR*>& values) const {
    auto sig = std::make_unique<Signature>();
    sig->selector = selector;

    for (auto nvid : arguments) {
        sig->argumentTypes.emplace_back(values[nvid]->value.typeFlags);
        sig->argumentClassNames.emplace_back(values[nvid]->value.knownClassName);
    }

    return sig;
}

void MessageHIR::addArgument(NVID id) {
    reads.emplace(id);
    arguments.emplace_back(id);
}

void MessageHIR::addKeywordArgument(NVID id) {
    reads.emplace(id);
    keywordArguments.emplace_back(id);
}

NVID MessageHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

bool MessageHIR::replaceInput(NVID original, NVID replacement) {
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

void MessageHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& vRegs,
        LIRList& append) const {
    // TODO: implement me
    append.emplace_back(std::make_unique<lir::LoadConstantLIR>(vReg(), Slot::makeNil()));
    vRegs[vReg()] = --(append.end());
}

} // namespace hir
} // namespace hadron
