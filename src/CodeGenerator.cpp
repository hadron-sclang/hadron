#include "hadron/CodeGenerator.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/JIT.hpp"
#include "hadron/SyntaxAnalyzer.hpp"
#include "Keywords.hpp"

namespace hadron {

CodeGenerator::CodeGenerator() {
    m_errorReporter = std::make_shared<ErrorReporter>(true);
}

CodeGenerator::CodeGenerator(std::shared_ptr<ErrorReporter> errorReporter): m_errorReporter(errorReporter) {}

bool CodeGenerator::jitBlock(const ast::BlockAST* /* block */, JIT* jit) {
    jit->prolog();
    /* insert magic here */
    jit->retr({ JIT::kNoSave, 0});
    jit->epilog();
    return true;
}

}  // namespace hadron