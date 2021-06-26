#include "CodeGenerator.hpp"

#include "Parser.hpp"

#include "fmt/core.h"
#include "spdlog/spdlog.h"

#include <vector>

namespace hadron {

CodeGenerator::CodeGenerator():
    m_context(std::make_unique<llvm::LLVMContext>()),
    m_builder(std::make_unique<llvm::IRBuilder<>>(*m_context)) { }

CodeGenerator::~CodeGenerator() {}

std::unique_ptr<llvm::Module> CodeGenerator::genInterpreterIR(const parse::Node* root, uint64_t uniqueID) {
    std::string moduleName = fmt::format("interp_{:016x}", uniqueID);
    auto module = std::make_unique<llvm::Module>(moduleName, *m_context);

    // We encode the IR for an interpreted block directly in the "main" entry point function of the Module.
    auto mainType = llvm::FunctionType::get(llvm::IntegerType::getInt32Ty(*m_context),
            std::vector<llvm::Type*>(), false);
    auto main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module.get());
    auto mainBasicBlock = llvm::BasicBlock::Create(*m_context, "entry", main);
    m_builder->SetInsertPoint(mainBasicBlock);

    buildIR(root);

    // Return 0 from main entry point function.
    m_builder->CreateRet(llvm::ConstantInt::get(*m_context, llvm::APInt(32, 0, true)));
    llvm::verifyFunction(*main);

    return module;
}

void CodeGenerator::buildIR(const parse::Node* node) {
    switch (node->nodeType) {
    case hadron::parse::NodeType::kEmpty:
        spdlog::error("CodeGenerator encountered empty parse node.");
        return;

    default:
        return;
    }
}

} // namespace hadron

