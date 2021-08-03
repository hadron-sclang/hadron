#include "hadron/SSABuilder.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Slot.hpp"


namespace hadron {

SSABuilder::SSABuilder(Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter):
    m_lexer(lexer), m_errorReporter(errorReporter), m_frame(nullptr), m_block(nullptr) { }

SSABuilder::~SSABuilder() { }

std::unique_ptr<Frame> SSABuilder::build(const Lexer* lexer, const parse::BlockNode* blockNode) {
    assert(blockNode);
    auto frame = std::make_unique<Frame>();
    // Make an entry block and add to frame.
    m_frame = frame.get();
    frame->blocks.emplace_back(std::make_unique<Block>(0));
    m_block = frame->blocks.begin()->get();

    // Build argument name list and default values.
    const parse::ArgListNode* argList = blockNode->arguments.get();
    int32_t argIndex = 0;
    while (argList) {
        assert(argList->nodeType == parse::NodeType::kArgList);
        const parse::VarListNode* varList = argList->varList.get();
        while (varList) {
            assert(varList->nodeType == parse::NodeType::kVarList);
            const parse::VarDefNode* varDef = varList->definitions.get();
            while (varDef) {
                assert(varDef->nodeType == parse::NodeType::kVarDef);
                auto name = lexer->tokens()[varDef->tokenIndex].hash;
                frame->argumentOrder.emplace_back(name);
                // TODO: for the ARG keyword the initial value can be *any* valid expr parse. For the | pair argument
                // delimiters valid syntax is any slotliteral, so basically anything that can be trivially packed
                // into a Slot. This seems a much more tractable way to describe initial values, so investigate
                // locking the parser down to only accept those for both argument styles. My suspcision is that
                // complex expressions using the `arg` syntax may parse but probably won't survive later compilation
                // stages in LSC anyway, so this may just represent making a de-facto LSC error into an explicit
                // parse error. Or, there's going to have to be an *if* block added to each variable in turn with
                // something like `if variable missing then do init expr` which just seems terrible.
                // For now we just do assignments with the LoadArgument opcode.


                auto loadArgValue = std::make_unique<hir::LoadArgumentHIR>(argIndex, true);

                auto loadArgType = std::make_unique<hir::LoadArgumentHIR>(argIndex, false);

                ++argIndex;
                varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
            }
            varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        }
        argList = reinterpret_cast<const parse::ArgListNode*>(argList->next.get());
    }

    // Variable definitions are allowed inline in Hadron, so we process variable definitions just like expression
    // sequences in the main body.
    if (blockNode->variables) {
        fillBlock(blockNode->variables.get());
    }
    if (blockNode->body) {
        fillBlock(blockNode->body.get());
    }

    return frame;
}

void SSABuilder::fillBlock(const parse::Node* node) {
    switch(node->nodeType) {
    case parse::NodeType::kEmpty:
        assert(false); // kEmpty is invalid node type.
        break;

    case parse::NodeType::kVarDef: {
        const auto varDef = reinterpret_cast<const parse::VarDefNode*>(node);
        auto nameToken = m_lexer->tokens()[varDef->tokenIndex];
        if (varDef->initialValue) {
            auto values = buildSSA(varDef->initialValue.get());
            m_block->nameRevisions[nameToken.hash] = values.first;
            m_block->typeRevisions[nameToken.hash] = values.ssecond;
        } else {
            std::unique_ptr<hir::HIR> initialValue = std::make_unique<
            std::unique_ptr<hir::HIR> initialType;

        }

        // TODO:: error reporting for variable redefinition

    } break;

    case parse::NodeType::kVarList: {
    } break;

    default: // DELETE ME
        break;
    }
}


} // namespace hadron