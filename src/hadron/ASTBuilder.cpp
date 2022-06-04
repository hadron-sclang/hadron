#include "hadron/ASTBuilder.hpp"

#include "hadron/ErrorReporter.hpp"
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

library::BlockAST ASTBuilder::buildBlock(ThreadContext* context, const library::BlockNode blockNode) {
    auto blockAST = library::BlockAST::makeBlock(context);

    auto argumentNames = blockAST.argumentNames();
    auto name = context->symbolTable->thisSymbol();
    // The *this* pointer is the first argument to every block.
    argumentNames = argumentNames.add(context, name);

    auto argumentDefaults = blockAST.argumentDefaults();
    argumentDefaults = argumentDefaults.add(context, Slot::makeNil());

    // Arguments with non-literal inits must be processed in the code as if blocks, after other variable definitions and
    // before the block body.
    std::vector<std::pair<library::Symbol, const library::Node>> exprInits;

    // Extract the rest of the arguments.
    auto argList = blockNode.arguments();
    if (argList) {
        auto varList = argList.varList();
        while (varList) {
            auto varDef = varList.definitions();
            while (varDef) {
                name = varDef.token().snippet(context);
                argumentNames = argumentNames.add(context, name);
                Slot initialValue = Slot::makeNil();
                if (varDef.initialValue()) {
                    // If the initial value is not a literal add it to the list of expressions to be processed at
                    // the top of the block.
                    if (!buildLiteral(context, varDef.initialValue().toBase(), initialValue)) {
                        exprInits.emplace_back(std::make_pair(name, varDef.initialValue()));
                    }
                }
                argumentDefaults = argumentDefaults.add(context, initialValue);
                varDef = library::VarDefNode(varDef.next().slot());
            }
            varList = library::VarListNode(varList.next().slot());
        }
        // There should be at most one arglist in a parse tree.
        assert(!argList.next());

        if (argList.varArgsNameToken()) {
            blockAST.setHasVarArg(true);
            name = argList.varArgsNameToken().snippet(context);
            argumentNames = argumentNames.add(context, name);
            argumentDefaults = argumentDefaults.add(context, Slot::makeNil());
        }
    }

    blockAST.setArgumentNames(argumentNames);
    blockAST.setArgumentDefaults(argumentDefaults);

    // We would like to eventually supprt inline variable declarations, so we process variable declarations like
    // ordinary expressions.
    auto blockStatements = blockAST.statements();
    appendToSequence(context, blockStatements, blockNode.variables().toBase());

    // Process any of the expression argument initializations here.
    for (const auto& init : exprInits) {
        auto isNilAST = library::MessageAST::makeMessage(context);
        isNilAST.setSelector(context->symbolTable->isNilSymbol());

        auto isNilNameAST = library::NameAST::makeName(context, init.first);
        isNilAST.arguments().addAST(context, isNilNameAST.toBase());

        auto ifAST = library::IfAST::makeIf(context);
        ifAST.condition().addAST(context, isNilAST.toBase());

        auto trueStatements = ifAST.trueBlock().statements();
        appendToSequence(context, trueStatements, init.second);
        ifAST.trueBlock().setStatements(trueStatements);

        // Default value of an empty else block is nil.
        ifAST.falseBlock().statements().addAST(context,
                library::ConstantAST::makeConstant(context, Slot::makeNil()).toBase());
    }

    // Append the expressions inside the parsed blockNode.
    appendToSequence(context, blockStatements, blockNode.body().toBase());

    blockAST.setStatements(blockStatements);
    return blockAST;
}

bool ASTBuilder::buildLiteral(ThreadContext* context, const library::Node node, Slot& literal) {
    switch(node.className()) {
    case library::SlotNode::nameHash(): {
        const auto slotNode = library::SlotNode(node.slot());
        literal = slotNode.value();
        return true;
    }

    case library::StringNode::nameHash(): {
        const auto stringNode = library::StringNode(node.slot());
        const auto token = stringNode.token();

        // Compute total length of string, to avoid recopies during concatenation.
        int32_t totalLength = token.length();
        auto nextNode = library::StringNode(stringNode.next().slot());
        while (nextNode) {
            totalLength += nextNode.token().length();
            nextNode = library::StringNode(nextNode.next().slot());
        }

        // Build the string from the individual components.
        auto string = library::String::arrayAlloc(context, totalLength);
        string = string.appendView(context, token.snippet(context).view(context), token.hasEscapeCharacters());

        nextNode = library::StringNode(stringNode.next().slot());
        while (nextNode) {
            const auto& nextToken = nextNode.token();
            string = string.appendView(context, nextToken.snippet(context).view(context),
                    nextToken.hasEscapeCharacters());
            nextNode = library::StringNode(nextNode.next().slot());
        }

        literal = string.slot();
        return true;
    }

    case library::SymbolNode::nameHash(): {
        const auto& token = node.token();
        auto string = library::String::arrayAlloc(context, token.length());
        string = string.appendView(context, token.snippet(context).view(context), token.hasEscapeCharacters());
        literal = library::Symbol::fromString(context, string).slot();
        return true;
    }

    default:
        literal = Slot::makeNil();
        return false;
    }
}

int32_t ASTBuilder::appendToSequence(ThreadContext* context, library::SequenceAST sequence, const library::Node node,
        int32_t startCurryCount) {
    int32_t curryCount = startCurryCount;
    auto seq = sequence.sequence();
    library::Node seqNode = node;

    while (seqNode) {
        auto ast = transform(context, seqNode, curryCount);

        if (ast.className() == library::SequenceAST::nameHash()) {
            library::SequenceAST sequenceAST(ast.slot());
            library::Array subSequence(sequenceAST.sequence());
            seq = seq.addAll(context, subSequence);
        } else {
            seq = seq.add(context, ast.slot());
        }
        seqNode = seqNode.next();
    }

    sequence.setSequence(seq);
    return curryCount;
}

library::AST ASTBuilder::transform(ThreadContext* context, const library::Node node, int32_t& curryCount) {

    switch(node.className()) {

    case library::ArgListNode::nameHash():
        assert(false); // internal error, not a valid node within a block
        return library::EmptyAST::alloc(context).toBase();

    // Transform into a array.at(index) message.
    case library::ArrayReadNode::nameHash(): {
        const auto readNode = library::ArrayReadNode(node.slot());
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(context->symbolTable->atSymbol());
        appendToSequence(context, message.arguments(), readNode.targetArray());
        appendToSequence(context, message.arguments(), readNode.indexArgument().toBase());
        return message.toBase();
    }

    // Transform into a array.put(index, value) message.
    case library::ArrayWriteNode::nameHash(): {
        const auto writeNode = library::ArrayWriteNode(node.slot());
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(context->symbolTable->putSymbol());
        appendToSequence(context, message.arguments(), writeNode.targetArray());
        appendToSequence(context, message.arguments(), writeNode.indexArgument().toBase());
        appendToSequence(context, message.arguments(), writeNode.value());
        return message.toBase();
    }

    case library::AssignNode::nameHash(): {
        const auto assignNode = library::AssignNode(node.slot());
        auto assign = library::AssignAST::makeAssign(context);
        assign.setName(assignNode.name().token().snippet(context));
        assign.setValue(transform(context, assignNode.value(), curryCount));
        return assign.toBase();
    }

    // Transform to a standard message with two arguments.
    case library::BinopCallNode::nameHash(): {
        const auto binop = library::BinopCallNode(node.slot());
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(binop.token().snippet(context));
        auto subCurryCount = appendToSequence(context, message.arguments(), binop.leftHand());
        subCurryCount = appendToSequence(context, message.arguments(), binop.rightHand(), subCurryCount);

        if (subCurryCount == 0) { return message.toBase(); }

        auto block = buildPartialBlock(context, subCurryCount);
        block.statements().addAST(context, library::AST::wrapUnsafe(message.slot()));
        return library::AST::wrapUnsafe(block.slot());
    }

    case library::BlockNode::nameHash(): {
        const auto blockNode = library::BlockNode(node.slot());
        auto block = buildBlock(context, blockNode);
        return library::AST::wrapUnsafe(block.slot());
    }

    case library::CallNode::nameHash(): {
        const auto callNode = library::CallNode(node.slot());
        return transformCallNode(context, callNode, callNode.token().snippet(context), curryCount);
    }

    case library::ClassNode::nameHash():
    case library::ClassExtNode::nameHash():
        assert(false); // internal error, not a valid node within a block
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());

    // Transform to a copySeries message.
    case library::CopySeriesNode::nameHash(): {
        const auto copySeriesNode = library::CopySeriesNode(node.slot());
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(context->symbolTable->copySeriesSymbol());
        appendToSequence(context, message.arguments(), copySeriesNode.target());
        appendToSequence(context, message.arguments(), copySeriesNode.first().toBase());

        // Provide second argument if present, otherwise default to nil.
        if (copySeriesNode.second()) {
            appendToSequence(context, message.arguments(), copySeriesNode.second());
        } else {
            auto nil = library::ConstantAST::makeConstant(context, Slot::makeNil());
            message.arguments().addAST(context, library::AST::wrapUnsafe(nil.slot()));
        }

        appendToSequence(context, message.arguments(), copySeriesNode.last().toBase());
        return library::AST::wrapUnsafe(message.slot());
    }

    case library::CurryArgumentNode::nameHash(): {
        auto curryArgName = library::Symbol::fromView(context, fmt::format("_curry{}", curryCount));
        ++curryCount;
        auto name = library::NameAST::makeName(context, curryArgName);
        return library::AST::wrapUnsafe(name.slot());
    }

    case library::EmptyNode::nameHash():
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());

    // Transform into currentEnvironment.at(key)
    case library::EnvironmentAtNode::nameHash(): {
        const auto envirAtNode = library::EnvironmentAtNode(node.slot());
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(context->symbolTable->atSymbol());

        auto currentEnv = library::NameAST::makeName(context, context->symbolTable->currentEnvironmentSymbol());
        message.arguments().addAST(context, library::AST::wrapUnsafe(currentEnv.slot()));

        auto key = library::ConstantAST::makeConstant(context, envirAtNode.token().snippet(context).slot());
        message.arguments().addAST(context, library::AST::wrapUnsafe(key.slot()));

        return library::AST::wrapUnsafe(message.slot());
    }

    // Transform into currentEnvironment.put(key, value)
    case library::EnvironmentPutNode::nameHash(): {
        const auto envirPutNode = library::EnvironmentPutNode(node.slot());
        auto message = library::MessageAST::makeMessage(context);
        message.setSelector(context->symbolTable->putSymbol());

        auto currentEnv = library::NameAST::makeName(context, context->symbolTable->currentEnvironmentSymbol());
        message.arguments().addAST(context, library::AST::wrapUnsafe(currentEnv.slot()));

        auto key = library::ConstantAST::makeConstant(context, envirPutNode.token().snippet(context).slot());
        message.arguments().addAST(context, library::AST::wrapUnsafe(key.slot()));

        message.arguments().addAST(context, transform(context, envirPutNode.value(), curryCount));

        return library::AST::wrapUnsafe(message.slot());
    }

    // Create a new Event, then add each pair with a call to newEvent.put()
    case library::EventNode::nameHash(): {
        const auto eventNode = library::EventNode(node.slot());

        auto rootAST = library::MessageAST::makeMessage(context);
        rootAST.setSelector(context->symbolTable->newSymbol());
        auto eventName = library::NameAST::makeName(context, context->symbolTable->eventSymbol());
        rootAST.arguments().addAST(context, library::AST::wrapUnsafe(eventName.slot()));

        auto elements = eventNode.elements();
        while (elements) {
            auto putMessage = library::MessageAST::makeMessage(context); // std::make_unique<ast::MessageAST>();
            putMessage.setSelector(context->symbolTable->putSymbol());

            // First argument is the target, which is the Event returned by the last put() or new() call.
            putMessage.arguments().addAST(context, library::AST::wrapUnsafe(rootAST.slot()));

            auto key = transformSequence(context, library::ExprSeqNode(elements.slot()), curryCount);
            putMessage.arguments().addAST(context, library::AST::wrapUnsafe(key.slot()));

            // Arguments are always expected in pairs.
            elements = elements.next();
            assert(elements);
            auto value = transformSequence(context, library::ExprSeqNode(elements.slot()), curryCount);
            putMessage.arguments().addAST(context, library::AST::wrapUnsafe(value.slot()));

            rootAST = putMessage;
            elements = elements.next();
        }

        return library::AST::wrapUnsafe(rootAST.slot());
    }

    case library::ExprSeqNode::nameHash(): {
        const auto exprSeq = library::ExprSeqNode(node.slot());
        auto sequence = transformSequence(context, exprSeq, curryCount);
        return library::AST::wrapUnsafe(sequence.slot());
    }

    case library::IfNode::nameHash(): {
        const auto ifNode = library::IfNode(node.slot());
        auto ifAST = library::IfAST::makeIf(context);
        appendToSequence(context, ifAST.condition(), ifNode.condition().toBase());
        ifAST.setTrueBlock(buildBlock(context, ifNode.trueBlock()));
        if (ifNode.elseBlock()) {
            ifAST.setFalseBlock(buildBlock(context, ifNode.elseBlock()));
        } else {
            auto nil = library::ConstantAST::makeConstant(context, Slot::makeNil());
            ifAST.falseBlock().statements().addAST(context, library::AST::wrapUnsafe(nil.slot()));
        }
        return library::AST::wrapUnsafe(ifAST.slot());
    }

    // KeyValue nodes get flattened into a sequence.
    case library::KeyValueNode::nameHash(): {
        const auto keyVal = library::KeyValueNode(node.slot());
        auto seq = library::SequenceAST::makeSequence(context);
        appendToSequence(context, seq, keyVal.key());
        appendToSequence(context, seq, keyVal.value());
        return library::AST::wrapUnsafe(seq.slot());
    }

    // Transform into className.new() followed by add() messages for each element.
    case library::CollectionNode::nameHash(): {
        const auto listNode = library::CollectionNode(node.slot());

        auto rootAST = library::MessageAST::makeMessage(context);
        rootAST.setSelector(context->symbolTable->newSymbol());

        // Provide 'Array' as the default class name if one was not provided.
        if (listNode.className()) {
            rootAST.arguments().addAST(context, transform(context, listNode.className().toBase(), curryCount));
        } else {
            auto arrayName = library::NameAST::makeName(context, context->symbolTable->arraySymbol());
            rootAST.arguments().addAST(context, library::AST::wrapUnsafe(arrayName.slot()));
        }

        int32_t subCurryCount = 0;

        auto elementNode = listNode.elements();
        while (elementNode) {
            auto addMessage = library::MessageAST::makeMessage(context);
            addMessage.setSelector(context->symbolTable->addSymbol());
            // Target is the object returned by the last call to the new() or add() message.
            addMessage.arguments().addAST(context, library::AST::wrapUnsafe(rootAST.slot()));

            auto element = transform(context, elementNode, subCurryCount);
            addMessage.arguments().addAST(context, element);

            rootAST = addMessage;
            elementNode = elementNode.next();
        }

        if (subCurryCount == 0) {
            return library::AST::wrapUnsafe(rootAST.slot());
        }

        auto block = buildPartialBlock(context, subCurryCount);
        block.statements().addAST(context, library::AST::wrapUnsafe(rootAST.slot()));
        return library::AST::wrapUnsafe(block.slot());
    }

    case library::MethodNode::nameHash():
        assert(false); // internal error, not a valid node within a block
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());

    case library::MultiAssignNode::nameHash(): {
        const auto multiAssignNode = library::MultiAssignNode(node.slot());
        auto multiAssignAST = library::MultiAssignAST::makeMultiAssign(context);
        multiAssignAST.setArrayValue(transform(context, multiAssignNode.value(), curryCount));

        auto nameNode = multiAssignNode.targets().names();
        while (nameNode) {
            auto targetName = library::NameAST::makeName(context, nameNode.token().snippet(context));
            multiAssignAST.targetNames().addAST(context, library::AST::wrapUnsafe(targetName.slot()));
            nameNode = library::NameNode(nameNode.next().slot());
        }

        if (multiAssignNode.targets().rest()) {
            auto restName = library::NameAST::makeName(context,
                    multiAssignNode.targets().rest().token().snippet(context));
            multiAssignAST.targetNames().addAST(context, library::AST::wrapUnsafe(restName.slot()));
            multiAssignAST.setLastIsRemain(true);
        }

        return library::AST::wrapUnsafe(multiAssignAST.slot());
    }

    case library::MultiAssignVarsNode::nameHash():
        // Should not be encountered on its own, but only within a MultiAssignNode.
        assert(false);
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());

    case library::NameNode::nameHash(): {
        const auto nameNode = library::NameNode(node.slot());
        auto name = library::NameAST::makeName(context, nameNode.token().snippet(context));
        return library::AST::wrapUnsafe(name.slot());
    }

    case library::NewNode::nameHash(): {
        const auto callNode = library::CallNode::wrapUnsafe(node.slot());
        return transformCallNode(context, callNode, context->symbolTable->newSymbol(), curryCount);
    }

    case library::PerformListNode::nameHash(): {
        const auto callNode = library::CallNode::wrapUnsafe(node.slot());
        return transformCallNode(context, callNode, context->symbolTable->performListSymbol(), curryCount);
    }

    case library::ReturnNode::nameHash(): {
        const auto returnNode = library::ReturnNode(node.slot());
        assert(returnNode.valueExpr());
        auto methodReturn = library::MethodReturnAST::makeMethodReturn(context);
        methodReturn.setValue(transform(context, returnNode.valueExpr(), curryCount));
        return library::AST::wrapUnsafe(methodReturn.slot());
    }

    case library::SeriesNode::nameHash(): {
        assert(false); // TODO
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());
    }

    case library::SeriesIterNode::nameHash(): {
        assert(false); // TODO
        return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());
    }

    // target.selector = value transforms to target.selector_(value)
    case library::SetterNode::nameHash(): {
        const auto setter = library::SetterNode(node.slot());
        auto message = library::MessageAST::makeMessage(context);
        auto subCurryCount = appendToSequence(context, message.arguments(), setter.target());
        auto selector = library::Symbol::fromView(context, fmt::format("{}_",
                setter.token().snippet(context).view(context)));
        message.setSelector(selector);
        subCurryCount = appendToSequence(context, message.arguments(), setter.value(), subCurryCount);

        if (subCurryCount == 0) {
            return library::AST::wrapUnsafe(message.slot());
        }

        auto block = buildPartialBlock(context, subCurryCount);
        block.statements().addAST(context, library::AST::wrapUnsafe(message.slot()));
        return library::AST::wrapUnsafe(block.slot());
    }

    case library::SlotNode::nameHash():
    case library::StringNode::nameHash():
    case library::SymbolNode::nameHash(): {
        Slot value = Slot::makeNil();
        bool wasLiteral = buildLiteral(context, node, value);
        assert(wasLiteral);
        auto constant = library::ConstantAST::makeConstant(context, value);
        return library::AST::wrapUnsafe(constant.slot());
    }

    case library::ValueNode::nameHash(): {
        const auto callNode = library::CallNode::wrapUnsafe(node.slot());
        return transformCallNode(context, callNode, context->symbolTable->valueSymbol(), curryCount);
    }

    case library::VarDefNode::nameHash(): {
        const auto varDef = library::VarDefNode(node.slot());
        auto defineAST = library::DefineAST::makeDefine(context);

        auto name = varDef.token().snippet(context);
        defineAST.setName(name);

        if (varDef.initialValue()) {
            defineAST.setValue(transform(context, varDef.initialValue(), curryCount));
        } else {
            auto nil = library::ConstantAST::makeConstant(context, Slot::makeNil());
            defineAST.setValue(library::AST::wrapUnsafe(nil.slot()));
        }
        return library::AST::wrapUnsafe(defineAST.slot());
    }

    case library::VarListNode::nameHash(): {
        const auto varList = library::VarListNode(node.slot());
        assert(varList.definitions());
        auto sequenceAST = library::SequenceAST::makeSequence(context);
        appendToSequence(context, sequenceAST, varList.definitions().toBase());
        return library::AST::wrapUnsafe(sequenceAST.slot());
    }

    case library::WhileNode::nameHash(): {
        const auto whileNode = library::WhileNode(node.slot());
        auto whileAST = library::WhileAST::makeWhile(context);
        // TODO: could be list comprehension nodes here, handle.
        whileAST.setConditionBlock(buildBlock(context, whileNode.conditionBlock()));
        if (whileNode.actionBlock()) {
            whileAST.setRepeatBlock(buildBlock(context, whileNode.actionBlock()));
        } else {
            auto nil = library::ConstantAST::makeConstant(context, Slot::makeNil());
            whileAST.repeatBlock().statements().addAST(context, library::AST::wrapUnsafe(nil.slot()));
        }
        return library::AST::wrapUnsafe(whileAST.slot());
    }
    }

    // Should not get here, likely a case is missing a return statement.
    assert(false);
    return library::AST::wrapUnsafe(library::EmptyAST::alloc(context).slot());
}

library::SequenceAST ASTBuilder::transformSequence(ThreadContext* context, const library::ExprSeqNode exprSeqNode,
        int32_t& curryCount) {
    auto sequenceAST = library::SequenceAST::makeSequence(context);
    if (!exprSeqNode || !exprSeqNode.expr()) { return sequenceAST; }
    curryCount = appendToSequence(context, sequenceAST, exprSeqNode.expr(), curryCount);
    return sequenceAST;
}

library::BlockAST ASTBuilder::buildPartialBlock(ThreadContext* context, int32_t numberOfArguments) {
    auto blockAST = library::BlockAST::makeBlock(context);
    assert(numberOfArguments > 0);

    auto argumentNames = blockAST.argumentNames();
    auto argumentDefaults = blockAST.argumentDefaults();

    for (int32_t i = 0; i < numberOfArguments; ++i) {
        auto curryArgName = library::Symbol::fromView(context, fmt::format("_curry{}", i));
        argumentNames = argumentNames.add(context, curryArgName);
        argumentDefaults = argumentDefaults.add(context, Slot::makeNil());
    }

    blockAST.setArgumentNames(argumentNames);
    blockAST.setArgumentDefaults(argumentDefaults);

    return blockAST;
}

library::AST ASTBuilder::transformCallNode(ThreadContext* context, const library::CallNode callNode,
        library::Symbol selector, int32_t& curryCount) {
    auto message = library::MessageAST::makeMessage(context);
    curryCount = appendToSequence(context, message.arguments(), callNode.target(), curryCount);
    message.setSelector(selector);

    auto subCurryCount = appendToSequence(context, message.arguments(), callNode.arguments());

    appendToSequence(context, message.keywordArguments(), callNode.keywordArguments().toBase());

    if (subCurryCount == 0) {
        return library::AST::wrapUnsafe(message.slot());
    }

    // The presence of curried arguments in the call means this is a partial application, define an inline function.
    auto blockAST = buildPartialBlock(context, subCurryCount);
    blockAST.statements().addAST(context, library::AST::wrapUnsafe(message.slot()));
    return library::AST::wrapUnsafe(blockAST.slot());
}

} // namespace hadron