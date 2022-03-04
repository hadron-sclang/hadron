#ifndef SRC_HADRON_HIR_BLOCK_LITERAL_HIR_HPP_
#define SRC_HADRON_HIR_BLOCK_LITERAL_HIR_HPP_

#include "hadron/hir/HIR.hpp"

#include <memory>

namespace hadron {
namespace ast {
struct BlockAST;
}

struct Frame;
struct ThreadContext;

namespace hir {

struct BlockLiteralHIR : public HIR {
    BlockLiteralHIR();
    virtual ~BlockLiteralHIR() = default;

    std::unique_ptr<Frame> frame;

    NVID proposeValue(NVID id) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_BLOCK_LITERAL_HIR_HPP_