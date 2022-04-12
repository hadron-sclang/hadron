#include "hadron/hir/MessageHIR.hpp"

#include "hadron/lir/LoadConstantLIR.hpp"

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

void MessageHIR::lower(const std::vector<HIR*>& /* values */, LinearFrame* /* linearFrame */) const {
    // TODO: implement me
    assert(false);
}

} // namespace hir
} // namespace hadron
