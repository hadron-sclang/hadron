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
                    if (!buildLiteral(context, lexer, varDef->initialValue.get(), initialValue)) {
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

bool ASTBuilder::buildLiteral(ThreadContext* context, const Lexer* lexer, const parse::Node* node, Slot& literal) {
    switch(node->nodeType) {
    case parse::NodeType::kSlot: {
        const auto slotNode = reinterpret_cast<const parse::SlotNode*>(node);
        literal = slotNode->value;
        return true;
    }

    case parse::NodeType::kString: {
        const auto stringNode = reinterpret_cast<const parse::StringNode*>(node);
        const auto& token = lexer->tokens()[stringNode->tokenIndex];

        // Compute total length of string, to avoid recopies during concatenation.
        int32_t totalLength = token.range.size();
        const parse::Node* nextNode = stringNode->next.get();
        while (nextNode) {
            assert(nextNode->nodeType == parse::NodeType::kString);
            totalLength += lexer->tokens()[nextNode->tokenIndex].range.size();
            nextNode = nextNode->next.get();
        }

        // Build the string from the individual components.
        auto string = library::String::arrayAlloc(context, totalLength);
        string = string.appendView(context, token.range, token.escapeString);

        nextNode = stringNode->next.get();
        while (nextNode) {
            const auto& nextToken = lexer->tokens()[nextNode->tokenIndex];
            string = string.appendView(context, nextToken.range, nextToken.escapeString);
            nextNode = nextNode->next.get();
        }

        literal = string.slot();
        return true;
    }

    case parse::NodeType::kSymbol: {
        const auto& token = lexer->tokens()[node->tokenIndex];
        assert(token.name == Token::Name::kSymbol || token.name == Token::Name::kKeyword);
        auto string = library::String::arrayAlloc(context, token.range.size());
        string = string.appendView(context, token.range, token.escapeString);
        literal = library::Symbol::fromString(context, string).slot();
        return true;
    }

    default:
        literal = Slot::makeNil();
        return false;
    }
}

int32_t ASTBuilder::appendToSequence(ThreadContext* context, const Lexer* lexer, ast::SequenceAST* sequenceAST,
        const parse::Node* node, int32_t startCurryCount) {
    int32_t curryCounter = startCurryCount;

    while (node != nullptr) {
        auto ast = transform(context, lexer, node, curryCounter);

        if (node->nodeType == parse::NodeType::kCurryArgument) { ++curryCounter; }

        if (ast->astType == ast::ASTType::kSequence) {
            auto subSeq = reinterpret_cast<ast::SequenceAST*>(ast.get());
            sequenceAST->sequence.splice(sequenceAST->sequence.end(), subSeq->sequence);
        } else {
            sequenceAST->sequence.emplace_back(std::move(ast));
        }
        node = node->next.get();
    }

    return curryCounter;
}

std::unique_ptr<ast::AST> ASTBuilder::transform(ThreadContext* context, const Lexer* lexer, const parse::Node* node,
        int32_t curryCounter) {
    switch(node->nodeType) {

    case parse::NodeType::kArgList:
        assert(false); // internal error, not a valid node within a block
        return std::make_unique<ast::EmptyAST>();

    // Transform literal lists into Array.with(elements) call.
    case parse::NodeType::kArray: {
        const auto arrayNode = reinterpret_cast<const parse::ArrayNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        message->selector = context->symbolTable->withSymbol();
        message->arguments->sequence.emplace_back(std::make_unique<ast::NameAST>(context->symbolTable->arraySymbol()));
        appendToSequence(context, lexer, message->arguments.get(), arrayNode->elements.get());
        return message;
    }

    case parse::NodeType::kArrayRead: {
        const auto readNode = reinterpret_cast<const parse::ArrayReadNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        message->selector = context->symbolTable->atSymbol();
        appendToSequence(context, lexer, message->arguments.get(), readNode->targetArray.get());
        appendToSequence(context, lexer, message->arguments.get(), readNode->indexArgument.get());
        return message;
    }

    case parse::NodeType::kArrayWrite: {
        const auto writeNode = reinterpret_cast<const parse::ArrayWriteNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        message->selector = context->symbolTable->putSymbol();
        appendToSequence(context, lexer, message->arguments.get(), writeNode->targetArray.get());
        appendToSequence(context, lexer, message->arguments.get(), writeNode->indexArgument.get());
        appendToSequence(context, lexer, message->arguments.get(), writeNode->value.get());
        return message;
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

    case parse::NodeType::kBinopCall: {
        const auto binop = reinterpret_cast<const parse::BinopCallNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        message->selector = library::Symbol::fromView(context, lexer->tokens()[binop->tokenIndex].range);
        auto curryCount = appendToSequence(context, lexer, message->arguments.get(), binop->leftHand.get());
        curryCount = appendToSequence(context, lexer, message->arguments.get(), binop->rightHand.get(), curryCount);
        assert(!binop->adverb); // TODO
        assert(curryCount == binop->countCurriedArguments());

        if (curryCount == 0) {
            return message;
        }

        auto block = buildPartialBlock(context, curryCount);
        block->statements->sequence.emplace_back(std::move(message));
        return block;
    }

    case parse::NodeType::kBlock: {
        const auto blockNode = reinterpret_cast<const parse::BlockNode*>(node);
        return buildBlock(context, lexer, blockNode);
    }

    case parse::NodeType::kCall: {
        const auto call = reinterpret_cast<const parse::CallNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        appendToSequence(context, lexer, message->arguments.get(), call->target.get());
        message->selector = library::Symbol::fromView(context, lexer->tokens()[call->tokenIndex].range);

        auto curryCount = appendToSequence(context, lexer, message->arguments.get(), call->arguments.get());
        assert(curryCount == call->countCurriedArguments());

        appendToSequence(context, lexer, message->keywordArguments.get(), call->keywordArguments.get());

        if (curryCount == 0) {
            return message;
        }

        // The presence of curried arguments in the call means this is a partial application, define an inline function.
        auto block = buildPartialBlock(context, curryCount);
        block->statements->sequence.emplace_back(std::move(message));
        return block;
    }

    case parse::NodeType::kClass:
    case parse::NodeType::kClassExt:
        assert(false); // internal error, not a valid node within a block
        return std::make_unique<ast::EmptyAST>();

    case parse::NodeType::kCopySeries: {
        const auto copySeriesNode = reinterpret_cast<const parse::CopySeriesNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        message->selector = context->symbolTable->copySeriesSymbol();
        appendToSequence(context, lexer, message->arguments.get(), copySeriesNode->target.get());
        appendToSequence(context, lexer, message->arguments.get(), copySeriesNode->first.get());

        // Provide second argument if present, otherwise default to nil.
        if (copySeriesNode->second) {
            appendToSequence(context, lexer, message->arguments.get(), copySeriesNode->second.get());
        } else {
            message->arguments->sequence.emplace_back(std::make_unique<ast::ConstantAST>(Slot::makeNil()));
        }

        appendToSequence(context, lexer, message->arguments.get(), copySeriesNode->last.get());
        return message;
    }

    case parse::NodeType::kCurryArgument: {
        auto curryArgName = library::Symbol::fromView(context, fmt::format("_curry{}", curryCounter));
        return std::make_unique<ast::NameAST>(curryArgName);
    } break;

    case parse::NodeType::kEmpty:
        return std::make_unique<ast::EmptyAST>();

    // New events must first be created, then each pair added with a call to newEvent.put()
    case parse::NodeType::kEvent: {
        const auto eventNode = reinterpret_cast<const parse::EventNode*>(node);

        auto rootAST = std::make_unique<ast::MessageAST>();
        rootAST->selector = context->symbolTable->newSymbol();
        rootAST->arguments->sequence.emplace_back(std::make_unique<ast::NameAST>(context->symbolTable->eventSymbol()));

        const parse::Node* elements = eventNode->elements.get();
        while (elements) {
            auto putMessage = std::make_unique<ast::MessageAST>();
            putMessage->selector = context->symbolTable->putSymbol();

            // First argument is the target, which is the Event returned by the last put() or new() call.
            putMessage->arguments->sequence.emplace_back(std::move(rootAST));

            assert(elements->nodeType == parse::NodeType::kExprSeq);
            auto key = transformSequence(context, lexer, reinterpret_cast<const parse::ExprSeqNode*>(elements));
            assert(key);
            putMessage->arguments->sequence.emplace_back(std::move(key));

            // Arguments are always expected in pairs.
            elements = elements->next.get();
            assert(elements);
            assert(elements->nodeType == parse::NodeType::kExprSeq);
            auto value = transformSequence(context, lexer, reinterpret_cast<const parse::ExprSeqNode*>(elements));
            assert(value);
            putMessage->arguments->sequence.emplace_back(std::move(value));

            rootAST = std::move(putMessage);
            elements = elements->next.get();
        }

        return rootAST;
    }

    case parse::NodeType::kExprSeq: {
        const auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(node);
        return transformSequence(context, lexer, exprSeq);
    }

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

    // KeyValue nodes get flattened into a sequence.
    case parse::NodeType::kKeyValue: {
        const auto keyVal = reinterpret_cast<const parse::KeyValueNode*>(node);
        auto seq = std::make_unique<ast::SequenceAST>();
        appendToSequence(context, lexer, seq.get(), keyVal->key.get());
        appendToSequence(context, lexer, seq.get(), keyVal->value.get());
        return seq;
    }

    case parse::NodeType::kLiteralDict: {
        assert(false); // TODO
    } break;

    // Transform into className.new() followed by add() messages for each element.
    case parse::NodeType::kLiteralList: {
        const auto listNode = reinterpret_cast<const parse::LiteralListNode*>(node);

        auto rootAST = std::make_unique<ast::MessageAST>();
        rootAST->selector = context->symbolTable->newSymbol();

        // Provide Array as the default class name if one was not provided.
        if (listNode->className) {
            rootAST->arguments->sequence.emplace_back(transform(context, lexer, listNode->className.get()));
        } else {
            rootAST->arguments->sequence.emplace_back(std::make_unique<ast::NameAST>(
                    context->symbolTable->arraySymbol()));
        }

        int32_t curryCounter = 0;

        const parse::Node* elementNode = listNode->elements.get();
        while (elementNode) {
            auto addMessage = std::make_unique<ast::MessageAST>();
            addMessage->selector = context->symbolTable->addSymbol();
            // Target is the object returned by the last call to the new() or add() message.
            addMessage->arguments->sequence.emplace_back(std::move(rootAST));

            auto element = transform(context, lexer, elementNode, curryCounter);
            assert(element);
            addMessage->arguments->sequence.emplace_back(std::move(element));

            if (elementNode->nodeType == parse::NodeType::kCurryArgument) { ++curryCounter; }

            rootAST = std::move(addMessage);
            elementNode = elementNode->next.get();
        }

        assert(curryCounter == listNode->countCurriedArguments());

        if (curryCounter == 0) {
            return rootAST;
        }

        auto block = buildPartialBlock(context, curryCounter);
        block->statements->sequence.emplace_back(std::move(rootAST));
        return block;
    } break;

    case parse::NodeType::kMethod:
        assert(false); // internal error, not a valid node within a block
        return std::make_unique<ast::EmptyAST>();

    case parse::NodeType::kMultiAssign: {
        const auto multiAssignNode = reinterpret_cast<const parse::MultiAssignNode*>(node);
        auto multiAssignAST = std::make_unique<ast::MultiAssignAST>();
        multiAssignAST->arrayValue = transform(context, lexer, multiAssignNode->value.get());

        const parse::NameNode* nameNode = multiAssignNode->targets->names.get();
        while (nameNode) {
            multiAssignAST->targetNames.emplace_back(transformName(context, lexer, nameNode));
            if (nameNode->next) {
                assert(nameNode->next->nodeType == parse::NodeType::kName);
            }
            nameNode = reinterpret_cast<const parse::NameNode*>(nameNode->next.get());
        }

        if (multiAssignNode->targets->rest) {
            multiAssignAST->targetNames.emplace_back(transformName(context, lexer,
                    multiAssignNode->targets->rest.get()));
            multiAssignAST->lastIsRemain = true;
        }

        return multiAssignAST;
    } break;

    case parse::NodeType::kMultiAssignVars:
        // Should not be encountered on its own, but only within a MultiAssignNode.
        assert(false);
        return std::make_unique<ast::EmptyAST>();

    case parse::NodeType::kName: {
        const auto nameNode = reinterpret_cast<const parse::NameNode*>(node);
        return transformName(context, lexer, nameNode);
    }

    case parse::NodeType::kNew: {
        const auto newNode = reinterpret_cast<const parse::NewNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        message->selector = context->symbolTable->newSymbol();
        appendToSequence(context, lexer, message->arguments.get(), newNode->target.get());
        auto curryCount = appendToSequence(context, lexer, message->arguments.get(), newNode->arguments.get());
        appendToSequence(context, lexer, message->keywordArguments.get(), newNode->keywordArguments.get());
        assert(curryCount == newNode->countCurriedArguments());

        if (curryCount == 0) {
            return message;
        }

        auto block = buildPartialBlock(context, curryCount);
        block->statements->sequence.emplace_back(std::move(message));
        return block;
    }

    case parse::NodeType::kNumericSeries: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kPerformList: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kReturn: {
        const auto returnNode = reinterpret_cast<const parse::ReturnNode*>(node);
        assert(returnNode->valueExpr);
        auto methodReturn = std::make_unique<ast::MethodReturnAST>();
        methodReturn->value = transform(context, lexer, returnNode->valueExpr.get());
        return methodReturn;
    }

    case parse::NodeType::kSeries: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kSeriesIter: {
        assert(false); // TODO
    } break;

    // target.selector = value transforms to target.selector_(value)
    case parse::NodeType::kSetter: {
        const auto setter = reinterpret_cast<const parse::SetterNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        auto curryCount = appendToSequence(context, lexer, message->arguments.get(), setter->target.get());
        message->selector = library::Symbol::fromView(context,
                fmt::format("{}_", lexer->tokens()[setter->tokenIndex].range));
        curryCount = appendToSequence(context, lexer, message->arguments.get(), setter->value.get(), curryCount);

        assert(curryCount == setter->countCurriedArguments());

        if (curryCount == 0) {
            return message;
        }

        auto block = buildPartialBlock(context, curryCount);
        block->statements->sequence.emplace_back(std::move(message));
        return block;
    }

    case parse::NodeType::kSlot:
    case parse::NodeType::kString:
    case parse::NodeType::kSymbol: {
        Slot value = Slot::makeNil();
        bool wasLiteral = buildLiteral(context, lexer, node, value);
        assert(wasLiteral);
        return std::make_unique<ast::ConstantAST>(value);
    }

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

std::unique_ptr<ast::NameAST> ASTBuilder::transformName(ThreadContext* context, const Lexer* lexer,
        const parse::NameNode* nameNode) {
        assert(!nameNode->isGlobal); // TODO
        return std::make_unique<ast::NameAST>(library::Symbol::fromView(context,
                lexer->tokens()[nameNode->tokenIndex].range));
}

std::unique_ptr<ast::BlockAST> ASTBuilder::buildPartialBlock(ThreadContext* context, int32_t numberOfArguments) {
    auto block = std::make_unique<ast::BlockAST>();

    for (int32_t i = 0; i < numberOfArguments; ++i) {
        auto curryArgName = library::Symbol::fromView(context, fmt::format("_curry{}", i));
        block->argumentNames = block->argumentNames.add(context, curryArgName);
        block->argumentDefaults = block->argumentDefaults.add(context, Slot::makeNil());
    }

    return block;
}

} // namespace hadron