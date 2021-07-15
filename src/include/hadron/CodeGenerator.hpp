#ifndef SRC_INCLUDE_HADRON_CODE_GENERATOR_HPP_
#define SRC_INCLUDE_HADRON_CODE_GENERATOR_HPP_

#include "hadron/Hash.hpp"
#include "hadron/JIT.hpp"

#include <memory>
#include <unordered_map>

class RegisterAllocator;

namespace hadron {

class ErrorReporter;
class VirtualJIT;
namespace ast {
struct AST;
struct BlockAST;
} // namespace ast

// A code generator will generate code in a VirtualJIT object from a single AST block tree.
class CodeGenerator {
public:
    CodeGenerator(const ast::BlockAST* block, std::shared_ptr<ErrorReporter> errorReporter);
    CodeGenerator() = delete;
    ~CodeGenerator() = default;

    bool generate();

    const VirtualJIT* virtualJIT() const { return m_jit.get(); }

private:
    void jitStatement(const ast::AST* ast, RegisterAllocator* allocator);

    const ast::BlockAST* m_block;
    std::unique_ptr<VirtualJIT> m_jit;
    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unordered_map<hadron::Hash, hadron::JIT::Label> m_addresses;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_CODE_GENERATOR_HPP_