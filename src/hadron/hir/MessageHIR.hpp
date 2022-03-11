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
    std::vector<NVID> arguments;
    std::vector<NVID> keywordArguments;

    std::unique_ptr<Signature> signature(const std::vector<HIR*>& values) const;
    void addArgument(NVID arg);
    void addKeywordArgument(NVID arg);

    NVID proposeValue(NVID id) override;
    bool replaceInput(NVID original, NVID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_MESSAGE_HIR_HPP_