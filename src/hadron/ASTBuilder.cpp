#include "hadron/ASTBuilder.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/library/String.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SymbolTable.hpp"

#include "fmt/format.h"

#include <cassert>

namespace hadron {

ASTBuilder::ASTBuilder(): m_errorReporter(std::make_shared<ErrorReporter>()) {}

ASTBuilder::ASTBuilder(std::shared_ptr<ErrorReporter> errorReporter): m_errorReporter(errorReporter) {}

std::unique_ptr<ast::BlockAST> ASTBuilder::buildBlock(ThreadContext* context, const Lexer* lexer,
        const parse::BlockNode* blockNode) {
    auto blockAST = std::make_unique<ast::BlockAST>();
    auto name = context->symbolTable->thisSymbol();

    // The *this* pointer is the first argument to every block.
    blockAST->argumentNames = blockAST->argumentNames.add(context, name);
    blockAST->argumentDefaults = blockAST->argumentDefaults.add(context, Slot::makeNil());

    // Arguments with non-literal inits must be processed in the code as if blocks, after other variable definitions and
    // before the block body.
    std::vector<std::pair<library::Symbol, const parse::Node*>> exprInits;

    // Extract the rest of the arguments.
    const parse::ArgListNode* argList = blockNode->arguments.get();
    if (argList) {
        assert(argList->nodeType == parse::NodeType::kArgList);
        const parse::VarListNode* varList = argList->varList.get();
        while (varList) {
            assert(varList->nodeType == parse::NodeType::kVarList);
            const parse::VarDefNode* varDef = varList->definitions.get();
            while (varDef) {
                assert(varDef->nodeType == parse::NodeType::kVarDef);
                name = library::Symbol::fromView(context, lexer->tokens()[varDef->tokenIndex].range);
                blockAST->argumentNames = blockAST->argumentNames.add(context, name);
                Slot initialValue = Slot::makeNil();
                if (varDef->initialValue) {
                    if (varDef->initialValue->nodeType == parse::NodeType::kLiteral) {
                        const auto literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
                        initialValue = literal->value;
                    } else {
                        exprInits.emplace_back(std::make_pair(name, varDef->initialValue.get()));
                    }
                }
                blockAST->argumentDefaults = blockAST->argumentDefaults.add(context, initialValue);
                varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
            }
            varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        }
        // There should be at most one arglist in a parse tree.
        assert(argList->next == nullptr);

        if (argList->varArgsNameIndex) {
            blockAST->hasVarArg = true;
            name = library::Symbol::fromView(context, lexer->tokens()[argList->varArgsNameIndex.value()].range);
            blockAST->argumentNames = blockAST->argumentNames.add(context, name);
            blockAST->argumentDefaults = blockAST->argumentDefaults.add(context, Slot::makeNil());
        }
    }

    // We would like to eventually supprt inline variable declarations, so we process variable declarations like
    // ordinary expressions.
    appendToSequence(context, lexer, blockAST->statements.get(), blockNode->variables.get());

    // Process any of the expression argument initializations here.
    for (const auto& init : exprInits) {
        auto isNil = std::make_unique<ast::MessageAST>();
        isNil->arguments->sequence.emplace_back(std::make_unique<ast::NameAST>(init.first));
        isNil->selector = context->symbolTable->isNilSymbol();

        auto ifAST = std::make_unique<ast::IfAST>();
        ifAST->condition->sequence.emplace_back(std::move(isNil));

        ifAST->trueBlock = std::make_unique<ast::BlockAST>();
        appendToSequence(context, lexer, ifAST->trueBlock->statements.get(), init.second);

        ifAST->falseBlock = std::make_unique<ast::BlockAST>();
        ifAST->falseBlock->statements->sequence.emplace_back(std::make_unique<ast::ConstantAST>(Slot::makeNil()));
    }

    // Append the expressions inside the parsed blockNode.
    appendToSequence(context, lexer, blockAST->statements.get(), blockNode->body.get());

    return blockAST;
}

void ASTBuilder::appendToSequence(ThreadContext* context, const Lexer* lexer, ast::SequenceAST* sequenceAST,
        const parse::Node* node) {
    while (node != nullptr) {
        auto ast = transform(context, lexer, node);
        if (ast->astType == ast::ASTType::kSequence) {
            auto subSeq = reinterpret_cast<ast::SequenceAST*>(ast.get());
            sequenceAST->sequence.splice(sequenceAST->sequence.end(), subSeq->sequence);
        } else {
            sequenceAST->sequence.emplace_back(std::move(ast));
        }
        node = node->next.get();
    }
}

std::unique_ptr<ast::AST> ASTBuilder::transform(ThreadContext* context, const Lexer* lexer, const parse::Node* node) {
    switch(node->nodeType) {
    case parse::NodeType::kEmpty:
        return std::make_unique<ast::EmptyAST>();

    case parse::NodeType::kVarDef: {
        const auto varDef = reinterpret_cast<const parse::VarDefNode*>(node);
        auto name = library::Symbol::fromView(context, lexer->tokens()[varDef->tokenIndex].range);
        auto defineAST = std::make_unique<ast::DefineAST>();
        defineAST->name = std::make_unique<ast::NameAST>(name);
        if (varDef->initialValue) {
            defineAST->value = transform(context, lexer, varDef->initialValue.get());
        } else {
            defineAST->value = std::make_unique<ast::ConstantAST>(Slot::makeNil());
        }
        return defineAST;
    }

    case parse::NodeType::kVarList: {
        const auto varList = reinterpret_cast<const parse::VarListNode*>(node);
        assert(varList->definitions);
        auto seq = std::make_unique<ast::SequenceAST>();
        appendToSequence(context, lexer, seq.get(), varList->definitions.get());
        return seq;
    }

    case parse::NodeType::kArgList:
    case parse::NodeType::kMethod:
    case parse::NodeType::kClassExt:
    case parse::NodeType::kClass:
        assert(false); // internal error, not a valid node within a block
        return std::make_unique<ast::EmptyAST>();

    case parse::NodeType::kReturn: {
        const auto returnNode = reinterpret_cast<const parse::ReturnNode*>(node);
        assert(returnNode->valueExpr);
        auto methodReturn = std::make_unique<ast::MethodReturnAST>();
        methodReturn->value = transform(context, lexer, returnNode->valueExpr.get());
        return methodReturn;
    }

    // Transform literal lists into Array.with(elements) call.
    case parse::NodeType::kList: {
        const auto listNode = reinterpret_cast<const parse::ListNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        message->selector = context->symbolTable->withSymbol();
        message->arguments->sequence.emplace_back(std::make_unique<ast::NameAST>(context->symbolTable->arraySymbol()));
        appendToSequence(context, lexer, message->arguments.get(), listNode->elements.get());
        return message;
    }

    case parse::NodeType::kDictionary: {
        assert(false); // TODO
    }

    case parse::NodeType::kBlock: {
        const auto blockNode = reinterpret_cast<const parse::BlockNode*>(node);
        return buildBlock(context, lexer, blockNode);
    }

    case parse::NodeType::kLiteral: {
        const auto literal = reinterpret_cast<const parse::LiteralNode*>(node);
        if (literal->blockLiteral) {
            return buildBlock(context, lexer, literal->blockLiteral.get());
        }

        Slot value = literal->value;
/*        if (literal->type == LiteralType::kString) {
            value = library::String::fromView(context, lexer->tokens()[literal->tokenIndex].range).slot();
        } else */ if (literal->type == LiteralType::kSymbol) {
            value = library::Symbol::fromView(context, lexer->tokens()[literal->tokenIndex].range).slot();
        } else {
            // The only pointer-based constants allowed are Strings and Symbols.
            assert(!value.isPointer());
        }
        return std::make_unique<ast::ConstantAST>(value);
    }

    case parse::NodeType::kString: {
        const auto string = reinterpret_cast<const parse::StringNode*>(node);
        if ()
    }

    case parse::NodeType::kName: {
        const auto name = reinterpret_cast<const parse::NameNode*>(node);
        assert(!name->isGlobal); // TODO
        return std::make_unique<ast::NameAST>(library::Symbol::fromView(context,
                lexer->tokens()[name->tokenIndex].range));
    }

    case parse::NodeType::kExprSeq: {
        const auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(node);
        return transformSequence(context, lexer, exprSeq);
    }

    case parse::NodeType::kAssign: {
        const auto assignNode = reinterpret_cast<const parse::AssignNode*>(node);
        assert(assignNode->name);
        auto name = library::Symbol::fromView(context, lexer->tokens()[assignNode->name->tokenIndex].range);
        auto assign = std::make_unique<ast::AssignAST>();
        assign->name = std::make_unique<ast::NameAST>(name);
        assert(assignNode->value);
        assign->value = transform(context, lexer, assignNode->value.get());
        return assign;
    }

    // target.selector = value transforms to target.selector_(value)
    case parse::NodeType::kSetter: {
        const auto setter = reinterpret_cast<const parse::SetterNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        appendToSequence(context, lexer, message->arguments.get(), setter->target.get());
        message->selector = library::Symbol::fromView(context,
                fmt::format("{}_", lexer->tokens()[setter->tokenIndex].range));
        appendToSequence(context, lexer, message->arguments.get(), setter->value.get());
        return message;
    }

    // KeyValue nodes get flattened into a sequence.
    case parse::NodeType::kKeyValue: {
        const auto keyVal = reinterpret_cast<const parse::KeyValueNode*>(node);
        auto seq = std::make_unique<ast::SequenceAST>();
        appendToSequence(context, lexer, seq.get(), keyVal->key.get());
        appendToSequence(context, lexer, seq.get(), keyVal->value.get());
        return seq;
    }

    case parse::NodeType::kCall: {
        const auto call = reinterpret_cast<const parse::CallNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        appendToSequence(context, lexer, message->arguments.get(), call->target.get());
        message->selector = library::Symbol::fromView(context, lexer->tokens()[call->tokenIndex].range);
        appendToSequence(context, lexer, message->arguments.get(), call->arguments.get());
        appendToSequence(context, lexer, message->keywordArguments.get(), call->keywordArguments.get());
        return message;
    }

    case parse::NodeType::kBinopCall: {
        const auto binop = reinterpret_cast<const parse::BinopCallNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        message->selector = library::Symbol::fromView(context, lexer->tokens()[binop->tokenIndex].range);
        appendToSequence(context, lexer, message->arguments.get(), binop->leftHand.get());
        appendToSequence(context, lexer, message->arguments.get(), binop->rightHand.get());
        assert(!binop->adverb); // TODO
        return message;
    }

    case parse::NodeType::kPerformList: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kNumericSeries: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kCurryArgument: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kArrayRead: {
        const auto readNode = reinterpret_cast<const parse::ArrayReadNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        message->selector = context->symbolTable->atSymbol();
        appendToSequence(context, lexer, message->arguments.get(), readNode->targetArray.get());
        appendToSequence(context, lexer, message->arguments.get(), readNode->indexArgument.get());
        return message;
    } break;

    case parse::NodeType::kArrayWrite: {
        const auto writeNode = reinterpret_cast<const parse::ArrayWriteNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        message->selector = context->symbolTable->putSymbol();
        appendToSequence(context, lexer, message->arguments.get(), writeNode->targetArray.get());
        appendToSequence(context, lexer, message->arguments.get(), writeNode->indexArgument.get());
        appendToSequence(context, lexer, message->arguments.get(), writeNode->value.get());
        return message;
    } break;

    case parse::NodeType::kCopySeries: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kNew: {
        const auto newNode = reinterpret_cast<const parse::NewNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        message->selector = context->symbolTable->newSymbol();
        appendToSequence(context, lexer, message->arguments.get(), newNode->target.get());
        appendToSequence(context, lexer, message->arguments.get(), newNode->arguments.get());
        appendToSequence(context, lexer, message->keywordArguments.get(), newNode->keywordArguments.get());
        return message;
    } break;

    case parse::NodeType::kSeries: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kSeriesIter: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kLiteralList: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kLiteralDict: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kMultiAssignVars: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kMultiAssign: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kIf: {
        const auto ifNode = reinterpret_cast<const parse::IfNode*>(node);
        auto ifAST = std::make_unique<ast::IfAST>();
        appendToSequence(context, lexer, ifAST->condition.get(), ifNode->condition.get());
        ifAST->trueBlock = buildBlock(context, lexer, ifNode->trueBlock.get());
        if (ifNode->falseBlock) {
            ifAST->falseBlock = buildBlock(context, lexer, ifNode->falseBlock.get());
        } else {
            ifAST->falseBlock = std::make_unique<ast::BlockAST>();
            ifAST->falseBlock->statements->sequence.emplace_back(std::make_unique<ast::ConstantAST>(Slot::makeNil()));
        }
        return ifAST;
    }

    case parse::NodeType::kWhile: {
        const auto whileNode = reinterpret_cast<const parse::WhileNode*>(node);
        auto whileAST = std::make_unique<ast::WhileAST>();
        whileAST->condition = buildBlock(context, lexer, whileNode->blocks.get());
        if (whileNode->blocks->next) {
            assert(whileNode->blocks->next->nodeType == parse::NodeType::kBlock);
            const auto repeatBlock = reinterpret_cast<const parse::BlockNode*>(whileNode->blocks->next.get());
            whileAST->repeatBlock = buildBlock(context, lexer, repeatBlock);
        } else {
            whileAST->repeatBlock = std::make_unique<ast::BlockAST>();
            whileAST->repeatBlock->statements->sequence.emplace_back(std::make_unique<ast::ConstantAST>(
                    Slot::makeNil()));
        }
        return whileAST;
    }
    }

    // Should not get here, likely a case is missing a return statement.
    assert(false);
    return std::make_unique<ast::EmptyAST>();
}

std::unique_ptr<ast::SequenceAST> ASTBuilder::transformSequence(ThreadContext* context, const Lexer* lexer,
        const parse::ExprSeqNode* exprSeqNode) {
    auto sequenceAST = std::make_unique<ast::SequenceAST>();
    if (!exprSeqNode || !exprSeqNode->expr) { return sequenceAST; }
    appendToSequence(context, lexer, sequenceAST.get(), exprSeqNode->expr.get());
    return sequenceAST;
}

} // namespace hadron