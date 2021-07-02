#ifndef SRC_SYNTAX_ANALYZER_HPP_
#define SRC_SYNTAX_ANALYZER_HPP_

#include "Literal.hpp"
#include "Type.hpp"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace hadron {

class ErrorReporter;
class Parser;
namespace parse {
struct BinopCallNode;
struct BlockNode;
struct CallNode;
struct Node;
}

namespace ast {
    enum ASTType {
        kCalculate,  // Arithmetic or comparisons of two numbers, either float or int
        kAssign,     // Assign a value to a variable
        kBlock,      // Scoped block of code
        kDispatch,   // method call
        kValue,
        kConstant,

        // control flow
        kWhile,      // while loop
    };

    struct AST {
        AST(ASTType t): astType(t) {}
        AST() = delete;
        virtual ~AST() = default;

        ASTType astType;
        Type valueType = Type::kSlot;
    };

    struct CalculateAST : public AST {
        CalculateAST(uint64_t hash): AST(kCalculate), selector(hash) {}
        CalculateAST() = delete;
        virtual ~CalculateAST() = default;

        uint64_t selector;
        std::unique_ptr<AST> left;
        std::unique_ptr<AST> right;
    };

    struct ValueAST;

    struct Value {
        Value(std::string n): name(n) {}
        std::string name;
        std::vector<ValueAST*> revisions;
    };

    struct BlockAST : public AST {
        BlockAST(BlockAST* p): AST(kBlock), parent(p) {}
        BlockAST() = delete;
        virtual ~BlockAST() = default;

        BlockAST* parent;
        std::unordered_map<uint64_t, Value> arguments;
        std::unordered_map<uint64_t, Value> variables;
        std::vector<std::unique_ptr<AST>> statements;
    };

    struct ValueAST : public AST {
        ValueAST(uint64_t hash, BlockAST* block): AST(kValue), nameHash(hash), owningBlock(block) {}
        ValueAST() = delete;
        virtual ~ValueAST() = default;

        uint64_t nameHash = 0;
        BlockAST* owningBlock = nullptr;
        size_t revision = 0;
    };

    struct AssignAST : public AST {
        AssignAST(): AST(kAssign) {}
        virtual ~AssignAST() = default;

        std::unique_ptr<ValueAST> target;
        std::unique_ptr<AST> value;
    };

    struct ConstantAST : public AST {
        ConstantAST(const Literal& literal): AST(kConstant), value(literal) {
            valueType = value.type();
        }
        ConstantAST() = delete;
        virtual ~ConstantAST() = default;
        Literal value;
    };

    struct WhileAST : public AST {
        WhileAST(): AST(kWhile) {
            valueType = Type::kNil;
        }
        virtual ~WhileAST() = default;

        // while { condition } { action }
        std::unique_ptr<BlockAST> condition;
        std::unique_ptr<BlockAST> action;
    };

    struct DispatchAST : public AST {
        DispatchAST(): AST(kDispatch) {}
        virtual ~DispatchAST() = default;

        uint64_t selectorHash;
        std::string selector;
        std::vector<std::unique_ptr<AST>> arguments;
    };
} // namespace ast

// Produces an AST from a Parse Tree
class SyntaxAnalyzer {
public:
    SyntaxAnalyzer(std::shared_ptr<ErrorReporter> errorReporter);
    ~SyntaxAnalyzer() = default;

    bool buildAST(const Parser* parser);

    const ast::AST* ast() const { return m_ast.get(); }

private:
    // Create a new BlockAST from a parse tree BlockNode.
    std::unique_ptr<ast::BlockAST> buildBlock(const Parser* parser, const parse::BlockNode* blockNode,
        ast::BlockAST* parent);
    // Fill an existing AST with nodes from the parse tree.
    void fillAST(const Parser* parser, const parse::Node* parseNode, ast::BlockAST* block,
        std::vector<std::unique_ptr<ast::AST>>* ast);
    // Build an expression tree without appending to the block, although variables may be appended if they are defined.
    std::unique_ptr<ast::AST> buildExprTree(const Parser* parser, const parse::Node* parseNode, ast::BlockAST* block);

    // Calls can be control flow or method dispatches. Differentiate, assemble, and return.
    std::unique_ptr<ast::AST> buildCall(const Parser* parser, const parse::CallNode* callNode, ast::BlockAST* block);
    // Binops can be arithmetic functions or method dispatches. Differentiate, assemble, and return.
    std::unique_ptr<ast::AST> buildBinop(const Parser* parser, const parse::BinopCallNode* binopNode,
        ast::BlockAST* block);

    // Find a value within the Block tree, or return nullptr if not found.
    std::unique_ptr<ast::ValueAST> findValue(uint64_t nameHash, ast::BlockAST* block, bool addReference);

    std::shared_ptr<ErrorReporter> m_errorReporter;

    std::unique_ptr<ast::AST> m_ast;
};

} // namespace hadron

#endif // SRC_SYNTAX_ANALYZER_HPP_