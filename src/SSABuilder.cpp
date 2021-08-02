#include "hadron/SSABuilder.hpp"

#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"

#include <cassert>

namespace hadron {

SSABuilder::SSABuilder() { }

SSABuilder::~SSABuilder() { }

std::unique_ptr<Frame> SSABuilder::build(const Lexer* lexer, const parse::BlockNode* blockNode) {
    assert(blockNode);
    auto frame = std::make_unique<Frame>();

    // Build argument name list and default values.
    const parse::ArgListNode* argList = blockNode->arguments.get();
    while (argList) {
        assert(argList->nodeType == parse::NodeType::kArgList);
        const parse::VarListNode* varList = argList->varList.get();
        while (varList) {
            assert(varList->nodeType == parse::NodeType::kVarList);
            const parse::VarDefNode* varDef = varList->definitions.get();
            while (varDef) {
                assert(varDef->nodeType == parse::NodeType::kVarDef);
                frame->argumentOrder.emplace_back(lexer->tokens()[varDef->tokenIndex].hash);
//                if (varDef->initialValue) {
//
//                }
                varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
            }
            varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        }
        argList = reinterpret_cast<const parse::ArgListNode*>(argList->next.get());
    }

    return frame;
}

} // namespace hadron