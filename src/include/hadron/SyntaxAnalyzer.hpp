#ifndef SRC_SYNTAX_ANALYZER_HPP_
#define SRC_SYNTAX_ANALYZER_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Literal.hpp"
#include "hadron/Type.hpp"

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hadron {

class ErrorReporter;
class Lexer;
class Parser;
namespace parse {
struct BinopCallNode;
struct BlockNode;
struct CallNode;
struct ClassNode;
struct Node;
}

namespace ast {
    enum ASTType {
        kAssign,      // Assign a value to a variable
        kInlineBlock, // Like a block but hoists its variable defenitions to the parent Block
        kBlock,       // Scoped block of code
        kCalculate,   // Arithmetic or comparisons of two numbers, either float or int
        kConstant,
        kDispatch,    // method call
        kValue,
        kWhile,       // while loop
        kClass,       // class definition
        kLoadFromSlot,
        kSaveToSlot
    };

    struct AST {
        AST(ASTType t): astType(t) {}
        AST() = delete;
        virtual ~AST() = default;

        ASTType astType;
        Type valueType = Type::kSlot;
    };

    struct ValueAST;

    struct CalculateAST : public AST {
        CalculateAST(Hash hash): AST(kCalculate), selector(hash) {}
        CalculateAST() = delete;
        virtual ~CalculateAST() = default;

        Hash selector;
        // Must always be a value.
        std::unique_ptr<AST> left;
        // Can either be a Value or a Constant
        std::unique_ptr<AST> right;
    };

    struct Value {
        Value(std::string n): name(n) {}
        std::string name;
        std::list<ValueAST*> references;
    };

    struct BlockAST : public AST {
        BlockAST(BlockAST* p): AST(kBlock), parent(p) {}
        BlockAST() = delete;
        virtual ~BlockAST() = default;

        BlockAST* parent;
        std::unordered_map<Hash, Value> arguments;
        std::unordered_map<Hash, Value> variables;
        std::list<std::unique_ptr<AST>> statements;
    };

    struct InlineBlockAST : public AST {
        InlineBlockAST(): AST(kInlineBlock) {}
        virtual ~InlineBlockAST() = default;

        std::list<std::unique_ptr<AST>> statements;
    };

    // Represents something that needs to be live in a register for manipulation.
    struct ValueAST : public AST {
        ValueAST(Hash hash, BlockAST* block): AST(kValue), nameHash(hash), owningBlock(block) {}
        ValueAST() = delete;
        virtual ~ValueAST() = default;

        Hash nameHash = 0;
        BlockAST* owningBlock = nullptr;
        bool isWrite = false;
        bool canRelease = false;
    };

    // Store a typed Value (virtual register) into a slot.
    struct SaveToSlotAST : public AST {
        SaveToSlotAST(): AST(kSaveToSlot) {}
        virtual ~SaveToSlotAST() = default;

        std::unique_ptr<ValueAST> value;
    };

    struct AssignAST : public AST {
        AssignAST(): AST(kAssign) {}
        virtual ~AssignAST() = default;

        // target <- value
        std::unique_ptr<AST> value;
        std::unique_ptr<ValueAST> target;
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
        std::unique_ptr<AST> condition;
        std::unique_ptr<AST> action;
    };

    struct DispatchAST : public AST {
        DispatchAST(): AST(kDispatch) {}
        virtual ~DispatchAST() = default;

        Hash selectorHash;
        std::string selector;
        std::list<std::unique_ptr<AST>> arguments;
    };

    struct ClassAST : public AST {
        ClassAST(): AST(kClass) {}
        virtual ~ClassAST() = default;

        Hash nameHash;
        std::string name;
        Hash superClassHash;

        std::unordered_map<Hash, Value> variables;
        std::unordered_map<Hash, Value> classVariables;
        std::unordered_map<Hash, Literal> constants;
        std::unordered_map<Hash, std::unique_ptr<BlockAST>> methods;
        std::unordered_map<Hash, std::unique_ptr<BlockAST>> classMethods;
        // Values store their names in the struct. For the constants and methods we store the name here.
        std::unordered_map<Hash, std::string> names;
    };
} // namespace ast

// Produces an AST from a Parse Tree
class SyntaxAnalyzer {
public:
    SyntaxAnalyzer(Parser* parser, std::shared_ptr<ErrorReporter> errorReporter);
    // For testing, builds AST directly from code input string, suppresses error reporting.
    SyntaxAnalyzer(std::string_view code);
    ~SyntaxAnalyzer() = default;

    // Convert from Parse Tree, bringing in Control Flow and type deduction. Also automatic conversion of Binops to
    // lowered type-specific versions. This output tree is suitable for direct translation into C++ code, or with
    // subsequent passes is suitable for use in a CodeGenerator for JIT.
    bool buildAST();

    // Anytime a ValueAST is provably a known value, replace it in the tree with a ConstantAST.
    // bool propagateConstants();

    // Remove dead code.
    // bool removeDeadCode();

    // Drop unused variables
    // bool pruneUnusedVariables();

    const ast::AST* ast() const { return m_ast.get(); }

private:
    // Create a new BlockAST from a parse tree BlockNode.
    std::unique_ptr<ast::BlockAST> buildBlock(const parse::BlockNode* blockNode, ast::BlockAST* parent);
    std::unique_ptr<ast::InlineBlockAST> buildInlineBlock(const parse::BlockNode* blockNode, ast::BlockAST* parent);
    std::unique_ptr<ast::ClassAST> buildClass(const parse::ClassNode* classNode);
    // Fill an existing AST with nodes from the parse tree, searching within |block| for variable names
    void fillAST(const parse::Node* parseNode, ast::BlockAST* block, std::list<std::unique_ptr<ast::AST>>* ast);
    // Build an expression tree without appending to the block, although variables may be appended if they are defined.
    std::unique_ptr<ast::AST> buildExprTree(const parse::Node* parseNode, ast::BlockAST* block);
    // Calls can be control flow or method dispatches. Differentiate, assemble, and return.
    std::unique_ptr<ast::AST> buildCall(const parse::CallNode* callNode, ast::BlockAST* block);
    // Binops can be arithmetic functions or method dispatches. Differentiate, assemble, and return.
    std::unique_ptr<ast::AST> buildBinop(const parse::BinopCallNode* binopNode, ast::BlockAST* block);
    // Find a value within the Block tree, or return nullptr if not found. |isWrite| should be true if this is
    // a write to this value.
    std::unique_ptr<ast::ValueAST> findValue(Hash nameHash, ast::BlockAST* block, bool isWrite);

    std::unique_ptr<Parser> m_ownParser;
    Parser* m_parser;
    Lexer* m_lexer;
    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<ast::AST> m_ast;
};

} // namespace hadron

#endif // SRC_SYNTAX_ANALYZER_HPP_