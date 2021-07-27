#include "hadron/Function.hpp"

#include "hadron/LighteningJIT.hpp"
#include "hadron/SyntaxAnalyzer.hpp"

namespace hadron {

Function::Function(const ast::BlockAST* block):
    machineCode(nullptr) {
    numberOfArgs = block->arguments.size();
    if (numberOfArgs) {
        argumentNames = std::make_unique<Hash[]>(numberOfArgs);
        // we don't know their order right now from the BlockAST
        defaultValues = std::make_unique<Slot[]>(numberOfArgs);
        // since we don't know order we don't know values
        nameIndices.resize(numberOfArgs);
        // build map from names back to indices
    }
}

} // namespace hadron