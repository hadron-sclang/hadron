#ifndef SRC_CODE_GENERATOR_HPP_
#define SRC_CODE_GENERATOR_HPP_

#include "LLVM.hpp"

#include <memory>

namespace hadron {

namespace parse {
struct Node;
}

namespace gen {


// Transform parse tree nodes into code blocks and instructions, for control flow analysis, subsequent transformation
// into SSA form, and finishing IR code generation.
struct Block {

};
}  // namespace gen

// While LLVM is multi-threaded this CodeGenerator is not, so for multithreaded code generation each thread should use
// their own CodeGen object.
class CodeGenerator {
public:
    CodeGenerator();
    ~CodeGenerator();

    std::unique_ptr<llvm::Module> genInterpreterIR(const parse::Node* root, uint64_t uniqueID);

private:
    void buildIR(const parse::Node* node);

    std::unique_ptr<llvm::LLVMContext> m_context;
    std::unique_ptr<llvm::IRBuilder<>> m_builder;
};

} // namespace hadron

#endif // SRC_CODE_GENERATOR_HPP_
