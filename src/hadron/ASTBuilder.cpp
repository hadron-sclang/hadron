#include "hadron/ASTBuilder.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/library/HadronAST.hpp"
#include "hadron/library/String.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SymbolTable.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include <cassert>

namespace hadron {

ASTBuilder::ASTBuilder(): m_errorReporter(std::make_shared<ErrorReporter>()) {}

ASTBuilder::ASTBuilder(std::shared_ptr<ErrorReporter> errorReporter): m_errorReporter(errorReporter) {}

library::BlockAST ASTBuilder::buildBlock(ThreadContext* context, const Lexer* lexer,
        const parse::BlockNode* blockNode) {
    auto blockAST = library::BlockAST::makeBlock(context);

    auto argumentNames = blockAST.argumentNames();
    auto name = context->symbolTable->thisSymbol();
    // The *this* pointer is the first argument to every block.
    argumentNames = argumentNames.add(context, name);

    auto argumentDefaults = blockAST.argumentDefaults();
    argumentDefaults = argumentDefaults.add(context, Slot::makeNil());

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
                argumentNames = argumentNames.add(context, name);
                Slot initialValue = Slot::makeNil();
                if (varDef->initialValue) {
                    if (!buildLiteral(context, lexer, varDef->initialValue.get(), initialValue)) {
                        exprInits.emplace_back(std::make_pair(name, varDef->initialValue.get()));
                    }
                }
                argumentDefaults = argumentDefaults.add(context, initialValue);
                varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
            }
            varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        }
        // There should be at most one arglist in a parse tree.
        assert(argList->next == nullptr);

        if (argList->varArgsNameIndex) {
            blockAST.setHasVarArg(true);
            name = library::Symbol::fromView(context, lexer->tokens()[argList->varArgsNameIndex.value()].range);
            argumentNames = argumentNames.add(context, name);
            argumentDefaults = argumentDefaults.add(context, Slot::makeNil());
        }
    }

    blockAST.setArgumentNames(argumentNames);
    blockAST.setArgumentDefaults(argumentDefaults);

    // We would like to eventually supprt inline variable declarations, so we process variable declarations like
    // ordinary expressions.
    auto blockStatements = blockAST.statements();
    appendToSequence(context, lexer, blockStatements, blockNode->variables.get());

    // Process any of the expression argument initializations here.
    for (const auto& init : exprInits) {
        auto isNilAST = library::MessageAST::makeMessage(context);
        isNilAST.setSelector(context->symbolTable->isNilSymbol());

        auto isNilNameAST = library::NameAST::makeName(context, init.first);
        isNilAST.arguments().addAST(context, library::AST::wrapUnsafe(isNilNameAST.slot()));

        auto ifAST = library::IfAST::makeIf(context);
        ifAST.condition().addAST(context, library::AST::wrapUnsafe(isNilAST.slot()));

        auto trueStatements = ifAST.trueBlock().statements();
        appendToSequence(context, lexer, trueStatements, init.second);
        ifAST.trueBlock().setStatements(trueStatements);

        // Default value of an empty else block is nil.
        ifAST.falseBlock().statements().addAST(context, library::AST::wrapUnsafe(
                library::ConstantAST::makeConstant(context, Slot::makeNil()).slot()));
    }

    // Append the expressions inside the parsed blockNode.
    appendToSequence(context, lexer, blockStatements, blockNode->body.get());

    blockAST.setStatements(blockStatements);
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

int32_t ASTBuilder::appendToSequence(ThreadContext* context, const Lexer* lexer, library::SequenceAST sequence,
        const parse::Node* node, int32_t startCurryCount) {
    int32_t curryCount = startCurryCount;
    auto seq = sequence.sequence();

    while (node != nullptr) {
        auto ast = transform(context, lexer, node, curryCount);

        if (ast.className() == library::SequenceAST::nameHash()) {
            library::SequenceAST sequenceAST(ast.slot());
            library::Array subSequence(sequenceAST.sequence());
            seq = seq.addAll(context, subSequence);
        } else {
            seq = seq.add(context, ast.slot());
        }
        node = node->next.get();
    }

    sequence.setSequence(seq);
    return curryCount;
}

library::AST ASTBuilder::transform(ThreadContext* context, const Lexer* lexer, const parse::Node* node,
        int32_t& curryCount) {

    switch(node->nodeType) {

    case parse::NodeType::kArgList:
        assert(false); // internal error, not a valid node within a block
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());

    // Transform literal lists into Array.with(elements) call.
    case parse::NodeType::kArray: {
        const auto arrayNode = reinterpret_cast<const parse::ArrayNode*>(node);
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(context->symbolTable->withSymbol());
        auto nameAST = library::NameAST::makeName(context, context->symbolTable->arraySymbol());
        message.arguments().addAST(context, library::AST::wrapUnsafe(nameAST.slot()));
        appendToSequence(context, lexer, message.arguments(), arrayNode->elements.get());
        return library::AST::wrapUnsafe(message.slot());
    }

    // Transform into a array.at(index) message.
    case parse::NodeType::kArrayRead: {
        const auto readNode = reinterpret_cast<const parse::ArrayReadNode*>(node);
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(context->symbolTable->atSymbol());
        appendToSequence(context, lexer, message.arguments(), readNode->targetArray.get());
        appendToSequence(context, lexer, message.arguments(), readNode->indexArgument.get());
        return library::AST::wrapUnsafe(message.slot());
    }

    // Transform into a array.put(index, value) message.
    case parse::NodeType::kArrayWrite: {
        const auto writeNode = reinterpret_cast<const parse::ArrayWriteNode*>(node);
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(context->symbolTable->putSymbol());
        appendToSequence(context, lexer, message.arguments(), writeNode->targetArray.get());
        appendToSequence(context, lexer, message.arguments(), writeNode->indexArgument.get());
        appendToSequence(context, lexer, message.arguments(), writeNode->value.get());
        return library::AST::wrapUnsafe(message.slot());
    }

    case parse::NodeType::kAssign: {
        const auto assignNode = reinterpret_cast<const parse::AssignNode*>(node);
        assert(assignNode->name);
        auto name = library::Symbol::fromView(context, lexer->tokens()[assignNode->name->tokenIndex].range);
        auto assign = library::AssignAST::makeAssign(context);
        assign.setName(name);
        assert(assignNode->value);
        assign.setValue(transform(context, lexer, assignNode->value.get(), curryCount));
        return library::AST::wrapUnsafe(assign.slot());
    }

    // Transform to a standard message with two arguments.
    case parse::NodeType::kBinopCall: {
        const auto binop = reinterpret_cast<const parse::BinopCallNode*>(node);
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(library::Symbol::fromView(context, lexer->tokens()[binop->tokenIndex].range));
        auto subCurryCount = appendToSequence(context, lexer, message.arguments(), binop->leftHand.get());
        subCurryCount = appendToSequence(context, lexer, message.arguments(), binop->rightHand.get(),
                subCurryCount);
        assert(!binop->adverb); // TODO

        if (subCurryCount == 0) {
            return library::AST::wrapUnsafe(message.slot());
        }

        auto block = buildPartialBlock(context, subCurryCount);
        block.statements().addAST(context, library::AST::wrapUnsafe(message.slot()));
        return library::AST::wrapUnsafe(block.slot());
    }

    case parse::NodeType::kBlock: {
        const auto blockNode = reinterpret_cast<const parse::BlockNode*>(node);
        auto block = buildBlock(context, lexer, blockNode);
        return library::AST::wrapUnsafe(block.slot());
    }

    case parse::NodeType::kCall: {
        const auto call = reinterpret_cast<const parse::CallNode*>(node);
        return transformCallBase(context, lexer, call, library::Symbol::fromView(context,
                lexer->tokens()[call->tokenIndex].range), curryCount);
    }

    case parse::NodeType::kClass:
    case parse::NodeType::kClassExt:
        assert(false); // internal error, not a valid node within a block
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());

    // Transform to a copySeries message.
    case parse::NodeType::kCopySeries: {
        const auto copySeriesNode = reinterpret_cast<const parse::CopySeriesNode*>(node);
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(context->symbolTable->copySeriesSymbol());
        appendToSequence(context, lexer, message.arguments(), copySeriesNode->target.get());
        appendToSequence(context, lexer, message.arguments(), copySeriesNode->first.get());

        // Provide second argument if present, otherwise default to nil.
        if (copySeriesNode->second) {
            appendToSequence(context, lexer, message.arguments(), copySeriesNode->second.get());
        } else {
            auto nil = library::ConstantAST::makeConstant(context, Slot::makeNil());
            message.arguments().addAST(context, library::AST::wrapUnsafe(nil.slot()));
        }

        appendToSequence(context, lexer, message.arguments(), copySeriesNode->last.get());
        return library::AST::wrapUnsafe(message.slot());
    }

    case parse::NodeType::kCurryArgument: {
        auto curryArgName = library::Symbol::fromView(context, fmt::format("_curry{}", curryCount));
        ++curryCount;
        auto name = library::NameAST::makeName(context, curryArgName);
        return library::AST::wrapUnsafe(name.slot());
    }

    case parse::NodeType::kEmpty:
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());

    // Transform into currentEnvironment.at(key)
    case parse::NodeType::kEnvironmentAt: {
        const auto envirAtNode = reinterpret_cast<const parse::EnvironmentAtNode*>(node);
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(context->symbolTable->atSymbol());

        auto currentEnv = library::NameAST::makeName(context, context->symbolTable->currentEnvironmentSymbol());
        message.arguments().addAST(context, library::AST::wrapUnsafe(currentEnv.slot()));

        auto key = library::ConstantAST::makeConstant(context, library::Symbol::fromView(context,
                lexer->tokens()[envirAtNode->tokenIndex].range).slot());
        message.arguments().addAST(context, library::AST::wrapUnsafe(key.slot()));

        return library::AST::wrapUnsafe(message.slot());
    }

    // Transform into currentEnvironment.put(key, value)
    case parse::NodeType::kEnvironmentPut: {
        const auto envirPutNode = reinterpret_cast<const parse::EnvironmentPutNode*>(node);
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(context->symbolTable->putSymbol());

        auto currentEnv = library::NameAST::makeName(context, context->symbolTable->currentEnvironmentSymbol());
        message.arguments().addAST(context, library::AST::wrapUnsafe(currentEnv.slot()));

        auto key = library::ConstantAST::makeConstant(context, library::Symbol::fromView(context,
                lexer->tokens()[envirPutNode->tokenIndex].range).slot());
        message.arguments().addAST(context, library::AST::wrapUnsafe(key.slot()));

        message.arguments().addAST(context, transform(context, lexer, envirPutNode->value.get(), curryCount));

        return library::AST::wrapUnsafe(message.slot());
    }

    // Create a new Event, then add each pair with a call to newEvent.put()
    case parse::NodeType::kEvent: {
        const auto eventNode = reinterpret_cast<const parse::EventNode*>(node);

        auto rootAST = library::MessageAST::makeMessage(context);
        rootAST.setSelector(context->symbolTable->newSymbol());
        auto eventName = library::NameAST::makeName(context, context->symbolTable->eventSymbol());
        rootAST.arguments().addAST(context, library::AST::wrapUnsafe(eventName.slot()));

        const parse::Node* elements = eventNode->elements.get();
        while (elements) {
            auto putMessage = library::MessageAST::makeMessage(context); // std::make_unique<ast::MessageAST>();
            putMessage.setSelector(context->symbolTable->putSymbol());

            // First argument is the target, which is the Event returned by the last put() or new() call.
            putMessage.arguments().addAST(context, library::AST::wrapUnsafe(rootAST.slot()));

            assert(elements->nodeType == parse::NodeType::kExprSeq);
            auto key = transformSequence(context, lexer, reinterpret_cast<const parse::ExprSeqNode*>(elements),
                    curryCount);
            putMessage.arguments().addAST(context, library::AST::wrapUnsafe(key.slot()));

            // Arguments are always expected in pairs.
            elements = elements->next.get();
            assert(elements);
            assert(elements->nodeType == parse::NodeType::kExprSeq);
            auto value = transformSequence(context, lexer, reinterpret_cast<const parse::ExprSeqNode*>(elements),
                    curryCount);
            putMessage.arguments().addAST(context, library::AST::wrapUnsafe(value.slot()));

            rootAST = putMessage;
            elements = elements->next.get();
        }

        return library::AST::wrapUnsafe(rootAST.slot());
    }

    case parse::NodeType::kExprSeq: {
        const auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(node);
        auto sequence = transformSequence(context, lexer, exprSeq, curryCount);
        return library::AST::wrapUnsafe(sequence.slot());
    }

    case parse::NodeType::kIf: {
        const auto ifNode = reinterpret_cast<const parse::IfNode*>(node);
        auto ifAST = library::IfAST::makeIf(context);
        appendToSequence(context, lexer, ifAST.condition(), ifNode->condition.get());
        ifAST.setTrueBlock(buildBlock(context, lexer, ifNode->trueBlock.get()));
        if (ifNode->falseBlock) {
            ifAST.setFalseBlock(buildBlock(context, lexer, ifNode->falseBlock.get()));
        } else {
            auto nil = library::ConstantAST::makeConstant(context, Slot::makeNil());
            ifAST.falseBlock().statements().addAST(context, library::AST::wrapUnsafe(nil.slot()));
        }
        return library::AST::wrapUnsafe(ifAST.slot());
    }

    // KeyValue nodes get flattened into a sequence.
    case parse::NodeType::kKeyValue: {
        const auto keyVal = reinterpret_cast<const parse::KeyValueNode*>(node);
        auto seq = library::SequenceAST::makeSequence(context);
        appendToSequence(context, lexer, seq, keyVal->key.get());
        appendToSequence(context, lexer, seq, keyVal->value.get());
        return library::AST::wrapUnsafe(seq.slot());
    }

    case parse::NodeType::kLiteralDict: {
        assert(false); // TODO
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());
    } break;

    // Transform into className.new() followed by add() messages for each element.
    case parse::NodeType::kLiteralList: {
        const auto listNode = reinterpret_cast<const parse::LiteralListNode*>(node);

        auto rootAST = library::MessageAST::makeMessage(context);
        rootAST.setSelector(context->symbolTable->newSymbol());

        // Provide Array as the default class name if one was not provided.
        if (listNode->className) {
            rootAST.arguments().addAST(context, transform(context, lexer, listNode->className.get(), curryCount));
        } else {
            auto arrayName = library::NameAST::makeName(context, context->symbolTable->arraySymbol());
            rootAST.arguments().addAST(context, library::AST::wrapUnsafe(arrayName.slot()));
        }

        int32_t subCurryCount = 0;

        const parse::Node* elementNode = listNode->elements.get();
        while (elementNode) {
            auto addMessage = library::MessageAST::makeMessage(context);
            addMessage.setSelector(context->symbolTable->addSymbol());
            // Target is the object returned by the last call to the new() or add() message.
            addMessage.arguments().addAST(context, library::AST::wrapUnsafe(rootAST.slot()));

            auto element = transform(context, lexer, elementNode, subCurryCount);
            addMessage.arguments().addAST(context, element);

            rootAST = addMessage;
            elementNode = elementNode->next.get();
        }

        if (subCurryCount == 0) {
            return library::AST::wrapUnsafe(rootAST.slot());
        }

        auto block = buildPartialBlock(context, subCurryCount);
        block.statements().addAST(context, library::AST::wrapUnsafe(rootAST.slot()));
        return library::AST::wrapUnsafe(block.slot());
    } break;

    case parse::NodeType::kMethod:
        assert(false); // internal error, not a valid node within a block
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());

    case parse::NodeType::kMultiAssign: {
        const auto multiAssignNode = reinterpret_cast<const parse::MultiAssignNode*>(node);
        auto multiAssignAST = library::MultiAssignAST::makeMultiAssign(context);
        multiAssignAST.setArrayValue(transform(context, lexer, multiAssignNode->value.get(), curryCount));

        const parse::NameNode* nameNode = multiAssignNode->targets->names.get();
        while (nameNode) {
            auto targetName = library::NameAST::makeName(context, library::Symbol::fromView(context,
                    lexer->tokens()[nameNode->tokenIndex].range));
            multiAssignAST.targetNames().addAST(context, library::AST::wrapUnsafe(targetName.slot()));
            if (nameNode->next) {
                assert(nameNode->next->nodeType == parse::NodeType::kName);
            }
            nameNode = reinterpret_cast<const parse::NameNode*>(nameNode->next.get());
        }

        if (multiAssignNode->targets->rest) {
            auto restName = library::NameAST::makeName(context, library::Symbol::fromView(context,
                    lexer->tokens()[multiAssignNode->targets->rest->tokenIndex].range));
            multiAssignAST.targetNames().addAST(context, library::AST::wrapUnsafe(restName.slot()));
            multiAssignAST.setLastIsRemain(true);
        }

        return library::AST::wrapUnsafe(multiAssignAST.slot());
    } break;

    case parse::NodeType::kMultiAssignVars:
        // Should not be encountered on its own, but only within a MultiAssignNode.
        assert(false);
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());

    case parse::NodeType::kName: {
        const auto nameNode = reinterpret_cast<const parse::NameNode*>(node);
        auto name = library::NameAST::makeName(context, library::Symbol::fromView(context,
                lexer->tokens()[nameNode->tokenIndex].range));
        return library::AST::wrapUnsafe(name.slot());
    }

    case parse::NodeType::kNew: {
        const auto newNode = reinterpret_cast<const parse::NewNode*>(node);
        return transformCallBase(context, lexer, newNode, context->symbolTable->newSymbol(), curryCount);
    }

    case parse::NodeType::kNumericSeries: {
        assert(false); // TODO
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());
    } break;

    case parse::NodeType::kPerformList: {
        const auto performListNode = reinterpret_cast<const parse::PerformListNode*>(node);
        return transformCallBase(context, lexer, performListNode, context->symbolTable->performListSymbol(),
                curryCount);
    } break;

    case parse::NodeType::kReturn: {
        const auto returnNode = reinterpret_cast<const parse::ReturnNode*>(node);
        assert(returnNode->valueExpr);
        auto methodReturn = std::make_unique<ast::MethodReturnAST>();
        methodReturn->value = transform(context, lexer, returnNode->valueExpr.get(), curryCount);
        return methodReturn;
    }

    case parse::NodeType::kSeries: {
        assert(false); // TODO
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());
    } break;

    case parse::NodeType::kSeriesIter: {
        assert(false); // TODO
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());
    } break;

    // target.selector = value transforms to target.selector_(value)
    case parse::NodeType::kSetter: {
        const auto setter = reinterpret_cast<const parse::SetterNode*>(node);
        auto message = std::make_unique<ast::MessageAST>();
        auto subCurryCount = appendToSequence(context, lexer, message->arguments.get(), setter->target.get());
        message->selector = library::Symbol::fromView(context,
                fmt::format("{}_", lexer->tokens()[setter->tokenIndex].range));
        subCurryCount = appendToSequence(context, lexer, message->arguments.get(), setter->value.get(), subCurryCount);

        if (subCurryCount == 0) {
            return message;
        }

        auto block = buildPartialBlock(context, subCurryCount);
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

    case parse::NodeType::kValue: {
        const auto value = reinterpret_cast<const parse::ValueNode*>(node);
        return transformCallBase(context, lexer, value, context->symbolTable->valueSymbol(), curryCount);
    }

    case parse::NodeType::kVarDef: {
        const auto varDef = reinterpret_cast<const parse::VarDefNode*>(node);
        auto name = library::Symbol::fromView(context, lexer->tokens()[varDef->tokenIndex].range);
        auto defineAST = std::make_unique<ast::DefineAST>();
        defineAST->name = std::make_unique<ast::NameAST>(name);
        if (varDef->initialValue) {
            defineAST->value = transform(context, lexer, varDef->initialValue.get(), curryCount);
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
    return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());
}

std::unique_ptr<ast::SequenceAST> ASTBuilder::transformSequence(ThreadContext* context, const Lexer* lexer,
        const parse::ExprSeqNode* exprSeqNode, int32_t& curryCount) {
    auto sequenceAST = std::make_unique<ast::SequenceAST>();
    if (!exprSeqNode || !exprSeqNode->expr) { return sequenceAST; }
    curryCount = appendToSequence(context, lexer, sequenceAST.get(), exprSeqNode->expr.get(), curryCount);
    return sequenceAST;
}

std::unique_ptr<ast::BlockAST> ASTBuilder::buildPartialBlock(ThreadContext* context, int32_t numberOfArguments) {
    auto block = std::make_unique<ast::BlockAST>();
    assert(numberOfArguments > 0);

    for (int32_t i = 0; i < numberOfArguments; ++i) {
        auto curryArgName = library::Symbol::fromView(context, fmt::format("_curry{}", i));
        block->argumentNames = block->argumentNames.add(context, curryArgName);
        block->argumentDefaults = block->argumentDefaults.add(context, Slot::makeNil());
    }

    return block;
}

std::unique_ptr<ast::AST> ASTBuilder::transformCallBase(ThreadContext* context, const Lexer* lexer,
        const parse::CallBaseNode* callBaseNode, library::Symbol selector, int32_t& curryCount) {
    auto message = std::make_unique<ast::MessageAST>();
    curryCount = appendToSequence(context, lexer, message->arguments.get(), callBaseNode->target.get(), curryCount);
    message->selector = selector;

    auto subCurryCount = appendToSequence(context, lexer, message->arguments.get(), callBaseNode->arguments.get());

    appendToSequence(context, lexer, message->keywordArguments.get(), callBaseNode->keywordArguments.get());

    if (subCurryCount == 0) {
        return message;
    }

    // The presence of curried arguments in the call means this is a partial application, define an inline function.
    auto block = buildPartialBlock(context, subCurryCount);
    block->statements->sequence.emplace_back(std::move(message));
    return block;
}

} // namespace hadron