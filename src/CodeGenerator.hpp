#ifndef SRC_CODE_GENERATOR_HPP_
#define SRC_CODE_GENERATOR_HPP_

#include "Block.hpp"

#include <memory>

namespace hadron {

namespace parse {
struct BlockNode;
struct Node;
}

class CodeGenerator {
public:
    CodeGenerator() = default;
    ~CodeGenerator() = default;

    // We only accept Blocks as the root node for parse trees via this method.
    std::unique_ptr<Block> buildBlock(const parse::BlockNode* blockNode, Block* parent);

private:
    // For block->arguments nodes
    // void buildBlockArguments(const parse::Node* node, Block* block);
    // For Class/ClassExt and Method nodes?
    // void buildClassHIR();

    // Expr and ExprSeq can be used to produce a value, like in assignments, built inline to the provided block..
    // void buildExprValue(const parse::Node* node, Block* block, ValueRef targetValue);

    // For block internals, expecting ExprSeqs + variable definitions (not excluded, to leave support for
    // inline variable declaration support someday)
    void buildBlockHIR(const parse::Node* node, Block* block);

    int m_blockSerial = 0;
};

} // namespace hadron

#endif // SRC_CODE_GENERATOR_HPP_
