#ifndef SRC_SYNTAX_ANALYZER_HPP_
#define SRC_SYNTAX_ANALYZER_HPP_

#include <memory>

namespace hadron {

class ErrorReporter;
namespace parse {
struct Node;
}

namespace ast {
    enum Type {
        kAdd,       // add values
        kAssign,    // Assign a value to a variable
        kBlock,     // Scoped block of code
        kDispatch,  // method call
        kLessThan,  // compare values
        kWhile,     // while loop
    };

    struct AST {
        Type type;
    };

    struct BlockAST : public AST {
    };
}

// Produces an AST from a Parse Tree
class SyntaxAnalyzer {
public:
    SyntaxAnalyzer(std::shared_ptr<ErrorReporter> errorReporter);
    ~SyntaxAnalyzer() = default;

    bool buildAST(const parse::Node* root);

    const ast::AST* ast() const { return m_ast.get(); }

private:
    std::shared_ptr<ErrorReporter> m_errorReporter;

    std::unique_ptr<ast::AST> m_ast;
};

} // namespace hadron

#endif // SRC_SYNTAX_ANALYZER_HPP_