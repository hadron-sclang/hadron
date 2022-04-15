#ifndef SRC_HADRON_HIR_BLOCK_LITERAL_HIR_HPP_
#define SRC_HADRON_HIR_BLOCK_LITERAL_HIR_HPP_

#include "hadron/hir/HIR.hpp"

#include "hadron/library/Kernel.hpp"

#include <memory>

namespace hadron {
namespace ast {
struct BlockAST;
}

struct Frame;

namespace hir {

// BlockLiterals can possibly be inlined after their construction. If not, they are lowered to define Function objects,
// and the compiler adds the relevant FunctionDef as an element in the containing Method/FunctionDef's |selectors|
// array.
struct BlockLiteralHIR : public HIR {
    BlockLiteralHIR();
    virtual ~BlockLiteralHIR() = default;

    std::unique_ptr<Frame> frame;
    library::FunctionDef functionDef;

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(LinearFrame* linearFrame) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_BLOCK_LITERAL_HIR_HPP_