#ifndef SRC_HADRON_HIR_MESSAGE_HIR_HPP_
#define SRC_HADRON_HIR_MESSAGE_HIR_HPP_

#include "hadron/hir/HIR.hpp"

#include <vector>

namespace hadron {

struct Signature;

namespace hir {

struct MessageHIR : public HIR {
    MessageHIR();
    virtual ~MessageHIR() = default;

    library::Symbol selector;
    // Add arguments with the add* methods below.
    std::vector<ID> arguments;
    std::vector<ID> keywordArguments;

    void addArgument(ID arg);
    void addKeywordArgument(ID arg);

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(const std::vector<HIR*>& values, LinearFrame* linearFrame) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_MESSAGE_HIR_HPP_