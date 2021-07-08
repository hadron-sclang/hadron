#ifndef SRC_INCLUDE_HADRON_CODE_GENERATOR_HPP_
#define SRC_INCLUDE_HADRON_CODE_GENERATOR_HPP_

#include <memory>

namespace hadron {

class ErrorReporter;
class JIT;
namespace ast {
struct BlockAST;
} // namespace ast

class CodeGenerator {
public:
    CodeGenerator();
    CodeGenerator(std::shared_ptr<ErrorReporter> errorReporter);
    ~CodeGenerator() = default;

    bool jitBlock(const ast::BlockAST* block, JIT* jit);

private:
    std::shared_ptr<ErrorReporter> m_errorReporter;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_CODE_GENERATOR_HPP_