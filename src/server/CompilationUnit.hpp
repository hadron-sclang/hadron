#ifndef SRC_SERVER_COMPILATION_UNIT_HPP_
#define SRC_SERVER_COMPILATION_UNIT_HPP_

#include <memory>
#include <string>

namespace hadron {
struct Frame;
class Lexer;
struct LinearBlock;
class Parser;

namespace ast {
struct BlockAST;
}

namespace parse {
struct BlockNode;
} // namespace parse
} // namespace hadron

namespace server {

struct CompilationUnit {
    std::string name;
    std::shared_ptr<hadron::Lexer> lexer;
    std::shared_ptr<hadron::Parser> parser;

    const hadron::parse::BlockNode* blockNode;
    std::unique_ptr<hadron::ast::BlockAST> blockAST;
    std::unique_ptr<hadron::Frame> frame;
    std::unique_ptr<hadron::LinearBlock> linearBlock;
    std::unique_ptr<int8_t[]> byteCode;
    size_t byteCodeSize;
};

} // namespace server

#endif // SRC_SERVER_COMPILATION_UNIT_HPP_