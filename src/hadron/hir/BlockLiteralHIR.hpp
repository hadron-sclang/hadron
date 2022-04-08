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

// BlockLiterals can possibly be inlined after their construction. If not, they are lowered to define Function objects,
// and the compiler adds the relevant FunctionDef as an element in the containing Method/FunctionDef's |selectors|
// array.
struct BlockLiteralHIR : public HIR {
    BlockLiteralHIR() = delete;
    explicit BlockLiteralHIR(int32_t index);
    virtual ~BlockLiteralHIR() = default;

    // The index in the outer frame's |selectors| array of FunctionDefs.
    int32_t selectorIndex;
    std::unique_ptr<Frame> frame;

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(const std::vector<HIR*>& values, LinearFrame* linearFrame) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_BLOCK_LITERAL_HIR_HPP_