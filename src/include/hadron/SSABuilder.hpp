#ifndef SRC_INCLUDE_HADRON_SSA_BUILDER_HPP_
#define SRC_INCLUDE_HADRON_SSA_BUILDER_HPP_

#include <list>
#include <memory>

namespace hadron {

namespace parse {
struct BlockNode;
} // namespace parse

struct Block {
    int blockNumber;
    std::list<std::unique_ptr<HIR>> statements;
};

// Represents a stack frame, so can have arguments supplied, is a scope for local variables, has an entrance and exit
// Block.
struct Frame {
    Frame* parent;
    std::list<std::unique_ptr<Block>> blocks;
};

// Goes from parse tree to HIR in blocks of HIR in SSA form.
class SSABuilder {
public:
    SSABuilder();
    ~SSABuilder();

    std::unique_ptr<Frame> build(const parse::BlockNode* blockNode);

private:

};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_SSA_BUILDER_HPP_