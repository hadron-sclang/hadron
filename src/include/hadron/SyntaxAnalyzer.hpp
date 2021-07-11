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
        kSaveToSlot,

        // == lower-level concepts here
        kLoadAddress,     // Initialize an _addr variable with the correct address.
        kLoad,            // Load or store a value in memory
        kStore,
        kAliasRegister,   // Associate a virtual register with a Value
        kUnaliasRegister, // Mark virtual register as free
    };

    struct AST {
        AST(ASTType t): astType(t) {}
        AST() = delete;
        virtual ~AST() = default;

        ASTType astType;
        Type valueType = Type::kSlot;
    };

    struct CalculateAST : public AST {
        CalculateAST(Hash hash): AST(kCalculate), selector(hash) {}
        CalculateAST() = delete;
        virtual ~CalculateAST() = default;

        Hash selector;
        std::unique_ptr<AST> left;
        std::unique_ptr<AST> right;
    };

    struct ValueAST;

    struct Value {
        Value(std::string n): name(n) {}
        std::string name;
        std::list<ValueAST*> references;
    };

    struct BlockAST : public AST {
        BlockAST(BlockAST* p): AST(kBlock), parent(p) {}
        BlockAST() = delete;
        virtual ~BlockAST() = default;

        int numberOfVirtualRegisters = 0;
        int numberOfVirtualFloatRegisters = 0;
        int numberOfTemporaryVariables = 0;
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
        bool isLastReference = true;
        int registerNumber = -1;
        bool isAddress = false;
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

    struct LoadAddressAST: public AST {
        LoadAddressAST(std::unique_ptr<ValueAST> addr): AST(kLoadAddress), address(std::move(addr)) {}
        LoadAddressAST() = delete;
        virtual ~LoadAddressAST() = default;

        // this could just be a register number
        std::unique_ptr<ValueAST> address;
    };

    struct StoreAST : public AST {
        StoreAST(): AST(kStore) {}
        virtual ~StoreAST() = default;

        std::unique_ptr<ValueAST> address;
        std::unique_ptr<AST> offset;  // can be null, another non-address value, or a constant integer
        std::unique_ptr<ValueAST> value;  // must be a virtual register
    };

    struct AliasRegisterAST : public AST {
        AliasRegisterAST(int reg, Hash hash): AST(kAliasRegister), registerNumber(reg), nameHash(hash) {}
        AliasRegisterAST() = delete;
        virtual ~AliasRegisterAST() = default;

        int registerNumber;
        Hash nameHash;

        // Could have block of assigned virtual registers and time until next use?
    };

    struct UnaliasRegisterAST : public AST {
        UnaliasRegisterAST(int reg, Hash hash): AST(kUnaliasRegister), registerNumber(reg), nameHash(hash) {}
        UnaliasRegisterAST() = delete;
        virtual ~UnaliasRegisterAST() = default;

        int registerNumber;
        Hash nameHash;
    };

} // namespace ast

// Produces an AST from a Parse Tree
class SyntaxAnalyzer {
public:
    SyntaxAnalyzer(std::shared_ptr<ErrorReporter> errorReporter);
    ~SyntaxAnalyzer() = default;

    // Convert from Parse Tree, bringing in Control Flow and type deduction. Also automatic conversion of Binops to
    // lowered type-specific versions. This output tree is suitable for direct translation into C++ code, or with
    // subsequent passes is suitable for use in a CodeGenerator for JIT.
    bool buildAST(const Parser* parser);

    // Anytime a ValueAST is provably a known value, replace it in the tree with a ConstantAST.
    // bool propagateConstants();

    // Remove dead code.
    // bool removeDeadCode();

    // Drop unused variables
    // bool pruneUnusedVariables();

    // OK, the big problem is that tree manipulation is like the whole point of doing the syntax tree in the first
    // place, making things like type inference and constant propagation possible. But, every time the tree gets
    // modified the Value reference pointers and all of the state tracking within ValueAST get broken. So the idea
    // is to make ValueAST into something with lazy inialization, meaning it really just has a Type and a descendent
    // tree which is the initial value (if it's an ARG then the initial value is a loadArg operation and the type is
    // always Slot. If it's a variable than the initial value is always type of literal assigned, or a nil.
    // Type deduction can then happen, and it can track variable usage in internal tables on tree traversal.
    // Lazy initialization in C++ code gen means we only declare and initialize variables close to their use.
    // in JIT it means we try to allocate registers to values only when they are likely to be read in operations,
    // to keep the time a variable is "live" very small, and we only emit bytecode for loading values into those
    // values right before they are needed. Type deduction could probably happen on the first pass, still, just
    // with tables not stored in the actual tree.

    // For 3AF, thinking first pass converts AST to JIT bytecode using vector<int64_t> as backing store, and doing
    // virtual register allocation. We add commands to the JIT for allocating and freeing registers, which allows
    // machine-indendent VirtualJIT to allocate virtual registers, and then for CodeGenerator to convert that into
    // real JIT.

    // Lower tree to three-address form.
    void toThreeAddressForm();

    // Pack ValueASTs into virtual registers per Block, including maps and unmaps. Tree manipulation after this call
    // will break code generation.
    void assignVirtualRegisters();

    const ast::AST* ast() const { return m_ast.get(); }

private:
    // ==== buildAST helpers
    // Create a new BlockAST from a parse tree BlockNode.
    std::unique_ptr<ast::BlockAST> buildBlock(const Parser* parser, const parse::BlockNode* blockNode,
        ast::BlockAST* parent);
    std::unique_ptr<ast::InlineBlockAST> buildInlineBlock(const Parser* parser, const parse::BlockNode* blockNode,
        ast::BlockAST* parent);
    std::unique_ptr<ast::ClassAST> buildClass(const Parser* parser, const parse::ClassNode* classNode);
    // Fill an existing AST with nodes from the parse tree, searching within |block| for variable names
    void fillAST(const Parser* parser, const parse::Node* parseNode, ast::BlockAST* block,
        std::list<std::unique_ptr<ast::AST>>* ast);
    // Build an expression tree without appending to the block, although variables may be appended if they are defined.
    std::unique_ptr<ast::AST> buildExprTree(const Parser* parser, const parse::Node* parseNode, ast::BlockAST* block);
    // Calls can be control flow or method dispatches. Differentiate, assemble, and return.
    std::unique_ptr<ast::AST> buildCall(const Parser* parser, const parse::CallNode* callNode, ast::BlockAST* block);
    // Binops can be arithmetic functions or method dispatches. Differentiate, assemble, and return.
    std::unique_ptr<ast::AST> buildBinop(const Parser* parser, const parse::BinopCallNode* binopNode,
        ast::BlockAST* block);
    // Find a value within the Block tree, or return nullptr if not found. |isWrite| should be true if this is
    // a write to this value.
    std::unique_ptr<ast::ValueAST> findValue(Hash nameHash, ast::BlockAST* block, bool isWrite);

    // ==== toThreeAddressForm helpers
    void lowerTo3AF(ast::AST* ast, ast::BlockAST* block,
        std::list<std::unique_ptr<ast::AST>>::iterator currentStatement);
    // Always assume these are for writing first.
    std::unique_ptr<ast::ValueAST> makeNewTempValue(ast::BlockAST* block);
    std::unique_ptr<ast::ValueAST> findAddress(Hash nameHash, ast::BlockAST* block, bool isWrite);

    // ==== assignVirtualRegisters helpers
    void assignRegistersToBlock(ast::AST* ast, ast::BlockAST* block, std::unordered_map<Hash, int>& activeRegisters,
        std::unordered_set<int>& freeRegisters, std::list<std::unique_ptr<ast::AST>>::iterator currentStatement);

    std::shared_ptr<ErrorReporter> m_errorReporter;

    std::unique_ptr<ast::AST> m_ast;
};

} // namespace hadron

#endif // SRC_SYNTAX_ANALYZER_HPP_