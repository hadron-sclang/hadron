#include "hadron/Parser.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"

#include "doctest/doctest.h"
#include "spdlog/spdlog.h"

namespace hadron {

// root: classes | classextensions | cmdlinecode
TEST_CASE("Parser root") {
    SUBCASE("root: <e> (for interpreted code)") {
        Parser parser("");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        CHECK(parser.root()->nodeType == parse::NodeType::kEmpty);
        CHECK(parser.root()->tokenIndex == 0);
        CHECK(parser.root()->next.get() == nullptr);
        CHECK(parser.root()->tail == parser.root());
    }

    SUBCASE("root: <e> (for classes)") {
        Parser parser("");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        CHECK(parser.root()->nodeType == parse::NodeType::kEmpty);
        CHECK(parser.root()->tokenIndex == 0);
        CHECK(parser.root()->next.get() == nullptr);
        CHECK(parser.root()->tail == parser.root());
    }

    SUBCASE("root: classes") {
        Parser parser("A { } B { }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root()->next.get());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto className = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(className.name == Token::kClassName);
        CHECK(className.range.compare("A") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->variables == nullptr);
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->next != nullptr);
        REQUIRE(classNode->next->nodeType == parse::NodeType::kClass);
        classNode = reinterpret_cast<const parse::ClassNode*>(classNode->next.get());
        className = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(className.name == Token::kClassName);
        CHECK(className.range.compare("B") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->variables == nullptr);
        CHECK(classNode->methods == nullptr);
    }

    SUBCASE("root: classextensions") {
        Parser parser("+ A { } + B { }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClassExt);
        CHECK(parser.root()->tail == parser.root()->next.get());
        auto classExt = reinterpret_cast<const parse::ClassExtNode*>(parser.root());
        auto className = parser.lexer()->tokens()[classExt->tokenIndex];
        REQUIRE(className.name == Token::kClassName);
        CHECK(className.range.compare("A") == 0);
        CHECK(classExt->methods == nullptr);

        REQUIRE(classExt->next != nullptr);
        REQUIRE(classExt->next->nodeType == parse::NodeType::kClassExt);
        classExt = reinterpret_cast<const parse::ClassExtNode*>(classExt->next.get());
        className = parser.lexer()->tokens()[classExt->tokenIndex];
        REQUIRE(className.name == Token::kClassName);
        CHECK(className.range.compare("B") == 0);
        CHECK(classExt->methods == nullptr);
    }

    SUBCASE("root: cmdlinecode") {
        Parser parser("42");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);
        CHECK(block->tail == parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kSlot);
        auto slotNode = reinterpret_cast<const parse::SlotNode*>(block->body->expr.get());
        CHECK(slotNode->tokenIndex == 0);
        CHECK(slotNode->value.getInt32() == 42);
    }
}

// classdef: classname superclass '{' classvardecls methods '}'
//         | classname '[' optname ']' superclass '{' classvardecls methods '}'
TEST_CASE("Parser classdef") {
    SUBCASE("classdef: classname superclass '{' classvardecls methods '}'") {
        Parser parser("A : B { var x; a { } }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->next == nullptr);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("A") == 0);
        REQUIRE(classNode->superClassNameIndex);
        name = parser.lexer()->tokens()[classNode->superClassNameIndex.value()];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("B") == 0);
        CHECK(!classNode->optionalNameIndex);

        REQUIRE(classNode->variables);
        REQUIRE(classNode->variables->definitions);
        name = parser.lexer()->tokens()[classNode->variables->definitions->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("x") == 0);
        CHECK(classNode->variables->definitions->initialValue == nullptr);
        CHECK(classNode->variables->definitions->next == nullptr);

        REQUIRE(classNode->methods);
        name = parser.lexer()->tokens()[classNode->methods->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("a") == 0);
        CHECK(!classNode->methods->isClassMethod);
        REQUIRE(classNode->methods->body != nullptr);
        const auto method = classNode->methods->body.get();
        CHECK(method->arguments == nullptr);
        CHECK(method->variables == nullptr);
        CHECK(method->body == nullptr);
        CHECK(classNode->methods->next == nullptr);
    }

    SUBCASE("classdef: classname '[' optname ']' superclass '{' classvardecls methods '}'") {
        Parser parser("Sub [ opt ] : Super { const c = -5; *meth { } }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->next == nullptr);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("Sub") == 0);
        REQUIRE(classNode->optionalNameIndex);
        name = parser.lexer()->tokens()[classNode->optionalNameIndex.value()];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("opt") == 0);
        REQUIRE(classNode->superClassNameIndex);
        name = parser.lexer()->tokens()[classNode->superClassNameIndex.value()];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("Super") == 0);

        REQUIRE(classNode->variables);
        REQUIRE(classNode->variables->definitions);
        name = parser.lexer()->tokens()[classNode->variables->definitions->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("c") == 0);
        CHECK(classNode->variables->next == nullptr);

        REQUIRE(classNode->variables->definitions->initialValue);
        REQUIRE(classNode->variables->definitions->initialValue->nodeType == parse::NodeType::kSlot);
        auto slotNode = reinterpret_cast<const parse::SlotNode*>(
                classNode->variables->definitions->initialValue.get());
        CHECK(slotNode->value.getInt32() == -5);
        CHECK(classNode->variables->definitions->next == nullptr);

        REQUIRE(classNode->methods);
        name = parser.lexer()->tokens()[classNode->methods->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("meth") == 0);
        CHECK(classNode->methods->isClassMethod);
        REQUIRE(classNode->methods->body != nullptr);
        CHECK(classNode->methods->body->arguments == nullptr);
        CHECK(classNode->methods->body->variables == nullptr);
        CHECK(classNode->methods->body->body == nullptr);
        CHECK(classNode->methods->next == nullptr);
    }
}

// classextension: '+' classname '{' methods '}'
TEST_CASE("Parser classextension") {
    SUBCASE("classextension: '+' classname '{' methods '}'") {
        Parser parser("+ Cls { *classMethod {} method {} }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClassExt);
        CHECK(parser.root()->next == nullptr);
        auto classExt = reinterpret_cast<const parse::ClassExtNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classExt->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("Cls") == 0);

        REQUIRE(classExt->methods != nullptr);
        name = parser.lexer()->tokens()[classExt->methods->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("classMethod") == 0);
        CHECK(classExt->methods->isClassMethod);
        REQUIRE(classExt->methods->body != nullptr);
        CHECK(classExt->methods->body->arguments == nullptr);
        CHECK(classExt->methods->body->variables == nullptr);
        CHECK(classExt->methods->body->body == nullptr);

        REQUIRE(classExt->methods->next != nullptr);
        REQUIRE(classExt->methods->next->nodeType == parse::NodeType::kMethod);
        auto method = reinterpret_cast<const parse::MethodNode*>(classExt->methods->next.get());
        name = parser.lexer()->tokens()[method->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("method") == 0);
        CHECK(!method->isClassMethod);
        REQUIRE(method->body != nullptr);
        CHECK(method->body->arguments == nullptr);
        CHECK(method->body->variables == nullptr);
        CHECK(method->body->body == nullptr);
        CHECK(method->next == nullptr);
    }
}

// cmdlinecode: '(' funcvardecls1 funcbody ')'
//            | funcvardecls1 funcbody
//            | funcbody
TEST_CASE("Parser cmdlinecode") {
    SUBCASE("cmdlinecode: '(' funcvardecls1 funcbody ')'") {
        Parser parser("( var a; 0xa )");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        auto name = parser.lexer()->tokens()[block->variables->definitions->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("a") == 0);
        CHECK(block->variables->definitions->initialValue == nullptr);
        CHECK(block->variables->definitions->next == nullptr);
        CHECK(block->variables->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kSlot);
        auto slotNode = reinterpret_cast<const parse::SlotNode*>(block->body->expr.get());
        CHECK(slotNode->value.getInt32() == 0xa);
        CHECK(block->body->next == nullptr);
    }

    SUBCASE("cmdlinecode: funcvardecls1 funcbody") {
        Parser parser("var x = \\ex; x");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        auto nameToken = parser.lexer()->tokens()[block->variables->definitions->tokenIndex];
        REQUIRE(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("x") == 0);
        CHECK(block->variables->definitions->next == nullptr);
        CHECK(block->variables->next == nullptr);

        REQUIRE(block->variables->definitions->initialValue != nullptr);
        REQUIRE(block->variables->definitions->initialValue->nodeType == parse::NodeType::kSymbol);
        CHECK(block->variables->definitions->initialValue->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(block->body->expr.get());
        nameToken = parser.lexer()->tokens()[name->tokenIndex];
        REQUIRE(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("x") == 0);
        CHECK(name->next == nullptr);
    }

    SUBCASE("cmdlinecode: funcbody") {
        Parser parser("\"string\"");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kString);
        CHECK(parser.root()->next == nullptr);
    }
}

// classvardecls: <e> | classvardecls classvardecl
TEST_CASE("Parser classvardecls") {
    SUBCASE("classvardecls: <e>") {
        Parser parser("A { }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("A") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->variables == nullptr);
        CHECK(classNode->methods == nullptr);
    }

    SUBCASE("classvardecls: classvardecls classvardecl") {
        Parser parser("C { classvar a, b, c; var d, e, f; const g = 1, h = 2, i = 3; }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("C") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kClassVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("a") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("b") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("c") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);

        REQUIRE(varList->next != nullptr);
        REQUIRE(varList->next->nodeType == parse::NodeType::kVarList);
        varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kVar);

        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("d") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("e") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("f") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);

        REQUIRE(varList->next != nullptr);
        REQUIRE(varList->next->nodeType == parse::NodeType::kVarList);
        varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kConst);

        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("g") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getInt32() == 1);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("h") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getInt32() == 2);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("i") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getInt32() == 3);

        CHECK(varDef->next == nullptr);
        CHECK(varList->next == nullptr);
    }
}

// classvardecl: CLASSVAR rwslotdeflist ';'
//             | VAR rwslotdeflist ';'
//             | SC_CONST constdeflist ';'
TEST_CASE("Parser classvardecl") {
    SUBCASE("classvardecl: CLASSVAR rwslotdeflist ';'") {
        Parser parser("X { classvar <> a, > b, < c; }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("X") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kClassVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("a") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("b") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(!varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("c") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
    }

    SUBCASE("classvardecl: VAR rwslotdeflist ';'") {
        Parser parser("Y { var < d1, <> e2, > f3; }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("Y") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("d1") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("e2") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("f3") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
        CHECK(!varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);
    }

    SUBCASE("classvardecl: SC_CONST constdeflist ';'") {
        Parser parser("Z { const bogon = 42, <  red5 = \"goin' in\"; }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("Z") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kConst);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("bogon") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getInt32() == 42);
        CHECK(!varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("red5") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kString);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
    }
}

TEST_CASE("Parser methods") {
    SUBCASE("methods: <e>") {
        Parser parser("Zed { }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("Zed") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->methods == nullptr);
    }

    SUBCASE("methods: methods methoddef") {
        Parser parser("Multi { m { } ++ { } *x { } * * { } }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("Multi") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);

        REQUIRE(classNode->methods != nullptr);
        const parse::MethodNode* method = classNode->methods.get();
        name = parser.lexer()->tokens()[method->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("m") == 0);
        CHECK(!method->isClassMethod);
        CHECK(method->body != nullptr);

        REQUIRE(method->next != nullptr);
        REQUIRE(method->next->nodeType == parse::NodeType::kMethod);
        method = reinterpret_cast<const parse::MethodNode*>(method->next.get());
        name = parser.lexer()->tokens()[method->tokenIndex];
        REQUIRE(name.name == Token::kBinop);
        CHECK(name.range.compare("++") == 0);
        CHECK(!method->isClassMethod);
        CHECK(method->body != nullptr);

        REQUIRE(method->next != nullptr);
        REQUIRE(method->next->nodeType == parse::NodeType::kMethod);
        method = reinterpret_cast<const parse::MethodNode*>(method->next.get());
        name = parser.lexer()->tokens()[method->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("x") == 0);
        CHECK(method->isClassMethod);
        CHECK(method->body != nullptr);

        REQUIRE(method->next != nullptr);
        REQUIRE(method->next->nodeType == parse::NodeType::kMethod);
        method = reinterpret_cast<const parse::MethodNode*>(method->next.get());
        // This is an interesting parse, requiring a space between the class method indicator '*' and the binop '*'.
        // If the token is "**" that is parsed as a object binop method named "**".
        name = parser.lexer()->tokens()[method->tokenIndex];
        REQUIRE(name.name == Token::kAsterisk);
        CHECK(name.range.compare("*") == 0);
        CHECK(method->isClassMethod);
        CHECK(method->body != nullptr);
        CHECK(method->next == nullptr);
    }
}

TEST_CASE("Parser methoddef") {
    SUBCASE("methoddef: name '{' argdecls funcvardecls primitive methbody '}'") {
        Parser parser("W { m1 { |z| var c = z; _Prim; c; } }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("W") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);

        REQUIRE(classNode->methods != nullptr);
        const parse::MethodNode* method = classNode->methods.get();
        name = parser.lexer()->tokens()[method->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("m1") == 0);
        CHECK(!method->isClassMethod);
        REQUIRE(method->primitiveIndex);
        name = parser.lexer()->tokens()[method->primitiveIndex.value()];
        REQUIRE(name.name == Token::kPrimitive);
        CHECK(name.range.compare("_Prim") == 0);

        REQUIRE(method->body != nullptr);
        REQUIRE(method->body->arguments != nullptr);
        auto argList = method->body->arguments.get();
        REQUIRE(argList->varList != nullptr);
        REQUIRE(argList->varList->definitions != nullptr);
        name = parser.lexer()->tokens()[argList->varList->definitions->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("z") == 0);
        CHECK(argList->varList->definitions->initialValue == nullptr);
        CHECK(argList->varList->definitions->next == nullptr);

        REQUIRE(method->body->variables != nullptr);
        auto varList = method->body->variables.get();
        REQUIRE(varList->definitions != nullptr);
        name = parser.lexer()->tokens()[varList->definitions->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("c") == 0);
        REQUIRE(varList->definitions->initialValue != nullptr);
        REQUIRE(varList->definitions->initialValue->nodeType == parse::NodeType::kName);
        auto nameNode = reinterpret_cast<const parse::NameNode*>(varList->definitions->initialValue.get());
        name = parser.lexer()->tokens()[nameNode->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("z") == 0);

        REQUIRE(method->body->body != nullptr);
        REQUIRE(method->body->body->expr != nullptr);
        REQUIRE(method->body->body->expr->nodeType == parse::NodeType::kName);
        nameNode = reinterpret_cast<const parse::NameNode*>(method->body->body->expr.get());
        name = parser.lexer()->tokens()[nameNode->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("c") == 0);

        CHECK(classNode->next == nullptr);
    }

    SUBCASE("methoddef: binop '{' argdecls funcvardecls primitive methbody '}'") {
        Parser parser("Kz { +/+ { arg b, c; var m, n; _Thunk 17; } }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("Kz") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);

        REQUIRE(classNode->methods != nullptr);
        const parse::MethodNode* method = classNode->methods.get();
        name = parser.lexer()->tokens()[method->tokenIndex];
        REQUIRE(name.name == Token::kBinop);
        CHECK(name.range.compare("+/+") == 0);
        CHECK(!method->isClassMethod);
        REQUIRE(method->primitiveIndex);
        name = parser.lexer()->tokens()[method->primitiveIndex.value()];
        REQUIRE(name.name == Token::kPrimitive);
        CHECK(name.range.compare("_Thunk") == 0);

        REQUIRE(method->body != nullptr);
        REQUIRE(method->body->arguments != nullptr);
        auto argList = method->body->arguments.get();
        REQUIRE(argList->varList != nullptr);
        REQUIRE(argList->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = argList->varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("b") == 0);
        CHECK(varDef->initialValue == nullptr);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("c") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);

        REQUIRE(method->body->variables != nullptr);
        auto varList = method->body->variables.get();
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("m") == 0);
        CHECK(varDef->initialValue == nullptr);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("n") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);

        REQUIRE(method->body->body != nullptr);
        REQUIRE(method->body->body->expr != nullptr);
        REQUIRE(method->body->body->expr->nodeType == parse::NodeType::kSlot);
        auto literal = reinterpret_cast<const parse::SlotNode*>(method->body->body->expr.get());
        CHECK(literal->value.getInt32() == 17);

        CHECK(classNode->next == nullptr);
    }

    SUBCASE("methoddef: '*' name '{' argdecls funcvardecls primitive methbody '}'") {
        Parser parser("Mx { *clsMeth { |m=5, n=7| var k = 0; var z = \\sym; _X ^\\k } }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("Mx") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);

        REQUIRE(classNode->methods != nullptr);
        const parse::MethodNode* method = classNode->methods.get();
        name = parser.lexer()->tokens()[method->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("clsMeth") == 0);
        CHECK(method->isClassMethod);
        REQUIRE(method->primitiveIndex);
        name = parser.lexer()->tokens()[method->primitiveIndex.value()];
        REQUIRE(name.name == Token::kPrimitive);
        CHECK(name.range.compare("_X") == 0);

        REQUIRE(method->body != nullptr);
        REQUIRE(method->body->arguments != nullptr);
        auto argList = method->body->arguments.get();
        REQUIRE(argList->varList != nullptr);
        REQUIRE(argList->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = argList->varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("m") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getInt32() == 5);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("n") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getInt32() == 7);
        CHECK(varDef->next == nullptr);

        REQUIRE(method->body->variables != nullptr);
        const parse::VarListNode* varList = method->body->variables.get();
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("k") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getInt32() == 0);
        CHECK(varDef->next == nullptr);
        REQUIRE(varList->next != nullptr);
        REQUIRE(varList->next->nodeType == parse::NodeType::kVarList);
        varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("z") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSymbol);
        CHECK(varDef->next == nullptr);
        CHECK(varList->next == nullptr);

        REQUIRE(method->body->body != nullptr);
        REQUIRE(method->body->body->expr != nullptr);
        REQUIRE(method->body->body->expr->nodeType == parse::NodeType::kReturn);
        auto retNode = reinterpret_cast<const parse::ReturnNode*>(method->body->body->expr.get());
        REQUIRE(retNode->valueExpr != nullptr);
        REQUIRE(retNode->valueExpr->nodeType == parse::NodeType::kSymbol);

        CHECK(classNode->next == nullptr);
    }

    SUBCASE("methoddef: '*' binop '{' argdecls funcvardecls primitive methbody '}'") {
        Parser parser("QRS { * !== { arg x = nil, y = true; var sd; var mm; _Pz ^nil; } }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("QRS") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);

        REQUIRE(classNode->methods != nullptr);
        const parse::MethodNode* method = classNode->methods.get();
        name = parser.lexer()->tokens()[method->tokenIndex];
        REQUIRE(name.name == Token::kBinop);
        CHECK(name.range.compare("!==") == 0);
        CHECK(method->isClassMethod);
        REQUIRE(method->primitiveIndex);
        name = parser.lexer()->tokens()[method->primitiveIndex.value()];
        REQUIRE(name.name == Token::kPrimitive);
        CHECK(name.range.compare("_Pz") == 0);

        REQUIRE(method->body != nullptr);
        REQUIRE(method->body->arguments != nullptr);
        auto argList = method->body->arguments.get();
        REQUIRE(argList->varList != nullptr);
        REQUIRE(argList->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = argList->varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("x") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.isNil());
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("y") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getBool());
        CHECK(varDef->next == nullptr);

        REQUIRE(method->body->variables != nullptr);
        const parse::VarListNode* varList = method->body->variables.get();
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("sd") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
        REQUIRE(varList->next != nullptr);
        REQUIRE(varList->next->nodeType == parse::NodeType::kVarList);
        varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("mm") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
        CHECK(varList->next == nullptr);

        REQUIRE(method->body->body != nullptr);
        REQUIRE(method->body->body->expr != nullptr);
        REQUIRE(method->body->body->expr->nodeType == parse::NodeType::kReturn);
        auto retNode = reinterpret_cast<const parse::ReturnNode*>(method->body->body->expr.get());
        REQUIRE(retNode->valueExpr != nullptr);
        REQUIRE(retNode->valueExpr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(retNode->valueExpr.get());
        CHECK(literal->value.isNil());

        CHECK(classNode->next == nullptr);
    }
}

TEST_CASE("Parser funcvardecls1") {
    SUBCASE("funcvardecls1: funcvardecl") {
        Parser parser("var x;");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->body == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions != nullptr);
        auto varDef = block->variables->definitions.get();
        auto name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("x") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
        CHECK(block->variables->next == nullptr);
    }

    SUBCASE("funcvardecls1: funcvardecls1 funcvardecl") {
        Parser parser("var abc = 2; var d, e = 4, f;");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->body == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions != nullptr);
        const parse::VarDefNode* varDef = block->variables->definitions.get();
        auto name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("abc") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getInt32() == 2);
        CHECK(varDef->next == nullptr);

        REQUIRE(block->variables->next != nullptr);
        REQUIRE(block->variables->next->nodeType == parse::NodeType::kVarList);
        auto varList = reinterpret_cast<const parse::VarListNode*>(block->variables->next.get());
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("d") == 0);
        CHECK(varDef->initialValue == nullptr);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("e") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getInt32() == 4);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("f") == 0);
        CHECK(varDef->next == nullptr);

        CHECK(varList->next == nullptr);
    }
}

TEST_CASE("Parser funcvardecl") {
    SUBCASE("funcvardecl: VAR vardeflist ';'") {
        Parser parser("var first = \"abc\", second = \\zed, third;");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->body == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions != nullptr);
        const parse::VarDefNode* varDef = block->variables->definitions.get();
        auto name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("first") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kString);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("second") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSymbol);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("third") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
    }
}

// funretval: <e> | '^' expr optsemi
TEST_CASE("Parser funcbody") {
    SUBCASE("funcbody: funretval") {
        Parser parser("^false");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kReturn);
        const auto retNode = reinterpret_cast<const parse::ReturnNode*>(block->body->expr.get());
        REQUIRE(retNode->valueExpr != nullptr);
        REQUIRE(retNode->valueExpr->nodeType == parse::NodeType::kSlot);
        const auto literal = reinterpret_cast<const parse::SlotNode*>(retNode->valueExpr.get());
        CHECK(!literal->value.getBool());
        CHECK(retNode->next == nullptr);
    }

    SUBCASE("funcbody: exprseq funretval") {
        Parser parser("1; 'gar'; ^x");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body != nullptr);
        const auto exprSeq = block->body.get();

        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        const auto literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getInt32() == 1);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kSymbol);
        const auto symbol = reinterpret_cast<const parse::SymbolNode*>(literal->next.get());

        REQUIRE(symbol->next != nullptr);
        REQUIRE(symbol->next->nodeType == parse::NodeType::kReturn);
        auto retNode = reinterpret_cast<const parse::ReturnNode*>(symbol->next.get());
        REQUIRE(retNode->valueExpr != nullptr);
        REQUIRE(retNode->valueExpr->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(retNode->valueExpr.get());
        auto nameToken = parser.lexer()->tokens()[name->tokenIndex];
        CHECK(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("x") == 0);
        CHECK(name->next == nullptr);
    }
}

TEST_CASE("Parser rwslotdeflist") {
    SUBCASE("rwslotdeflist: rwslotdef") {
        Parser parser("M { var <> rw; }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("M") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("rw") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);
        CHECK(varDef->next == nullptr);

        CHECK(classNode->next == nullptr);
    }

    SUBCASE("rwslotdeflist ',' rwslotdef") {
        Parser parser("Cv { classvar a, < b, > c; }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("Cv") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->methods == nullptr);
        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kClassVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("a") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(!varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("b") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("c") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
        CHECK(!varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);
    }
}

// rwspec: <e> | '<' | READWRITEVAR | '>'
TEST_CASE("Parser rwslotdef") {
    SUBCASE("rwslotdef: rwspec name") {
        Parser parser("BFG { var prv_x; }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("BFG") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("prv_x") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(!varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
        CHECK(varDef->next == nullptr);

        CHECK(classNode->next == nullptr);
    }

    SUBCASE("rwslotdef: rwspec name '=' slotliteral") {
        Parser parser("Lit { var >ax = 2; }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("Lit") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("ax") == 0);
        CHECK(!varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);
        CHECK(varDef->next == nullptr);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        auto literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getInt32() == 2);

        CHECK(classNode->next == nullptr);
    }
}

// optcomma: <e> | ','
TEST_CASE("Parser constdeflist") {
    SUBCASE("constdeflist: constdef") {
        Parser parser("UniConst { const psi=\"psi\"; }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("UniConst") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kConst);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("psi") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kString);
        CHECK(!varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
    }

    SUBCASE("constdeflist: constdeflist optcomma constdef") {
        Parser parser("MultiConst { const a = -1.0 <b=2 < c = 3.0; }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("MultiConst") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kConst);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("a") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getFloat() == -1.0);
        CHECK(!varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("b") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getInt32() == 2);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("c") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getFloat() == 3.0);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
    }
}

// rspec: <e> | '<'
TEST_CASE("Parser constdef") {
    SUBCASE("constdef: rspec name '=' slotliteral") {
        Parser parser("Math { const <epsilon= -0.0001; }");
        REQUIRE(parser.parseClass());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        auto name = parser.lexer()->tokens()[classNode->tokenIndex];
        REQUIRE(name.name == Token::kClassName);
        CHECK(name.range.compare("Math") == 0);
        CHECK(!classNode->superClassNameIndex);
        CHECK(!classNode->optionalNameIndex);
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.lexer()->tokens()[varList->tokenIndex].name == Token::Name::kConst);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("epsilon") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kSlot);
        auto literal = reinterpret_cast<const parse::SlotNode*>(varDef->initialValue.get());
        CHECK(literal->value.getFloat() == -0.0001);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
    }
}

TEST_CASE("Parser vardeflist") {
    SUBCASE("vardeflist: vardef") {
        Parser parser("( var ax7; )");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        auto name = parser.lexer()->tokens()[block->variables->definitions->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("ax7") == 0);
        CHECK(block->variables->definitions->initialValue == nullptr);
        CHECK(block->variables->definitions->next == nullptr);
        CHECK(block->variables->next == nullptr);
        CHECK(block->body == nullptr);
    }

    SUBCASE("vardeflist: vardeflist ',' vardef") {
        Parser parser("( var m,n,o,p; )");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        const parse::VarDefNode* varDef = block->variables->definitions.get();
        auto name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("m") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("n") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("o") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("p") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
    }
}

TEST_CASE("Parser vardef") {
    SUBCASE("vardef: name") {
        Parser parser("( var very_long_name_with_numbers_12345; )");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        auto name = parser.lexer()->tokens()[block->variables->definitions->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("very_long_name_with_numbers_12345") == 0);
        CHECK(block->variables->definitions->initialValue == nullptr);
        CHECK(block->variables->definitions->next == nullptr);
        CHECK(block->variables->next == nullptr);
        CHECK(block->body == nullptr);
    }

    SUBCASE("vardef: name '=' expr") {
        Parser parser("( var x = -5.8; )");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        auto name = parser.lexer()->tokens()[block->variables->definitions->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("x") == 0);
        REQUIRE(block->variables->definitions->initialValue != nullptr);
        REQUIRE(block->variables->definitions->initialValue->nodeType == parse::NodeType::kSlot);
        auto literal = reinterpret_cast<const parse::SlotNode*>(block->variables->definitions->initialValue.get());
        CHECK(literal->value.getFloat() == -5.8);

        CHECK(block->variables->definitions->next == nullptr);
        CHECK(block->variables->next == nullptr);
        CHECK(block->body == nullptr);
    }

    SUBCASE("vardef: name '(' exprseq ')'") {
        Parser parser("( var seq(1; 2); )");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        auto name = parser.lexer()->tokens()[block->variables->definitions->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("seq") == 0);
        REQUIRE(block->variables->definitions->initialValue != nullptr);
        REQUIRE(block->variables->definitions->initialValue->nodeType == parse::NodeType::kExprSeq);
        auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(block->variables->definitions->initialValue.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getInt32() == 1);
        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(literal->next.get());
        CHECK(literal->value.getInt32() == 2);

        CHECK(exprSeq->next == nullptr);
        CHECK(block->variables->next == nullptr);
        CHECK(block->body == nullptr);
    }
}

TEST_CASE("Parser dictslotdef") {
    SUBCASE("dictslotdef: exprseq ':' exprseq") {
        Parser parser("(\"\": \"\")");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kEvent);
        auto dict = reinterpret_cast<const parse::EventNode*>(block->body->expr.get());
        REQUIRE(dict->elements);
        REQUIRE(dict->elements->expr);
        REQUIRE(dict->elements->expr->nodeType == parse::NodeType::kString);
        REQUIRE(dict->elements->next);
        REQUIRE(dict->elements->next->nodeType == parse::NodeType::kExprSeq);
        const auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(dict->elements->next.get());
        REQUIRE(exprSeq->expr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kString);
    }

    SUBCASE("dictslotdef: keybinop exprseq") {
        Parser parser("(foo: 4)");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kEvent);
        auto dict = reinterpret_cast<const parse::EventNode*>(block->body->expr.get());
        REQUIRE(dict->elements);
        REQUIRE(dict->elements->expr);
        REQUIRE(dict->elements->expr->nodeType == parse::NodeType::kSymbol);
        REQUIRE(dict->elements->next);
        REQUIRE(dict->elements->next->nodeType == parse::NodeType::kExprSeq);
        const auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(dict->elements->next.get());
        REQUIRE(exprSeq->expr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        const auto literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value == Slot::makeInt32(4));
    }
}

TEST_CASE("Parser dictslotlist") {
    SUBCASE("dictslotlist: %empty") {
        Parser parser("()");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kEvent);
        auto dict = reinterpret_cast<const parse::EventNode*>(block->body->expr.get());
        CHECK(dict->elements == nullptr);
    }

    SUBCASE("dictslotlist: dictslotlist1 optcomma") {
        Parser parser("(key: value, 4: 7,)");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kEvent);
        auto dict = reinterpret_cast<const parse::EventNode*>(block->body->expr.get());
        REQUIRE(dict->elements);

        REQUIRE(dict->elements->expr);
        REQUIRE(dict->elements->expr->nodeType == parse::NodeType::kSymbol);
        REQUIRE(dict->elements->next);
        REQUIRE(dict->elements->next->nodeType == parse::NodeType::kExprSeq);
        const parse::ExprSeqNode* exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(dict->elements->next.get());
        REQUIRE(exprSeq->expr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kName);
        const auto name = reinterpret_cast<const parse::NameNode*>(exprSeq->expr.get());
        CHECK(parser.lexer()->tokens()[name->tokenIndex].range.compare("value") == 0);
        REQUIRE(exprSeq->next);
        REQUIRE(exprSeq->next->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(exprSeq->next.get());
        REQUIRE(exprSeq->expr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value == Slot::makeInt32(4));
        REQUIRE(exprSeq->next);
        REQUIRE(exprSeq->next->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(exprSeq->next.get());
        REQUIRE(exprSeq->expr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value == Slot::makeInt32(7));
    }
}

TEST_CASE("Parser argdecls") {
    SUBCASE("argdecls: <e>") {
        Parser parser("{ 1 }");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const parse::BlockNode* block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kBlock);
        block = reinterpret_cast<const parse::BlockNode*>(block->body->expr.get());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kSlot);
        const auto literal = reinterpret_cast<const parse::SlotNode*>(block->body->expr.get());
        CHECK(literal->value.getInt32() == 1);
        CHECK(literal->next == nullptr);
    }

    SUBCASE("argdecls: ARG vardeflist ';'") {
        Parser parser("{ arg arg1, arg2, arg3; }");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const parse::BlockNode* block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kBlock);
        block = reinterpret_cast<const parse::BlockNode*>(block->body->expr.get());

        REQUIRE(block->arguments != nullptr);
        CHECK(!block->arguments->varArgsNameIndex);
        REQUIRE(block->arguments->varList != nullptr);

        REQUIRE(block->arguments->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = block->arguments->varList->definitions.get();
        auto name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("arg1") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("arg2") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("arg3") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
    }

    SUBCASE("argdecls: ARG vardeflist0 ELLIPSIS name ';'") {
        Parser parser("{ arg x, y, z ... w; }");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const parse::BlockNode* block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kBlock);
        block = reinterpret_cast<const parse::BlockNode*>(block->body->expr.get());
        REQUIRE(block->arguments != nullptr);
        REQUIRE(block->arguments->varArgsNameIndex);
        auto name = parser.lexer()->tokens()[block->arguments->varArgsNameIndex.value()];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("w") == 0);
        REQUIRE(block->arguments->varList != nullptr);

        REQUIRE(block->arguments->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = block->arguments->varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("x") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("y") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("z") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
    }

    SUBCASE("argdecls: ARG <e> ELLIPSIS name ';'") {
        Parser parser("{ arg ... args; }");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const parse::BlockNode* block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kBlock);
        block = reinterpret_cast<const parse::BlockNode*>(block->body->expr.get());

        REQUIRE(block->arguments != nullptr);
        REQUIRE(block->arguments->varArgsNameIndex);
        auto name = parser.lexer()->tokens()[block->arguments->varArgsNameIndex.value()];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("args") == 0);
        REQUIRE(block->arguments->varList != nullptr);
        CHECK(block->arguments->varList->definitions == nullptr);
    }

    SUBCASE("argdecls: '|' slotdeflist '|'") {
        Parser parser("{ |i,j,k| }");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const parse::BlockNode* block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kBlock);
        block = reinterpret_cast<const parse::BlockNode*>(block->body->expr.get());
        REQUIRE(block->arguments != nullptr);
        CHECK(!block->arguments->varArgsNameIndex);
        REQUIRE(block->arguments->varList != nullptr);

        REQUIRE(block->arguments->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = block->arguments->varList->definitions.get();
        auto name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("i") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("j") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("k") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
    }

    SUBCASE("argdecls: '|' slotdeflist0 ELLIPSIS name '|'") {
        Parser parser("{ |i0,j1,k2...w3| }");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const parse::BlockNode* block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kBlock);
        block = reinterpret_cast<const parse::BlockNode*>(block->body->expr.get());

        REQUIRE(block->arguments != nullptr);
        REQUIRE(block->arguments->varArgsNameIndex);
        auto name = parser.lexer()->tokens()[block->arguments->varArgsNameIndex.value()];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("w3") == 0);
        REQUIRE(block->arguments->varList != nullptr);

        REQUIRE(block->arguments->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = block->arguments->varList->definitions.get();
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("i0") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("j1") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        name = parser.lexer()->tokens()[varDef->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("k2") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
    }

    SUBCASE("argdecls: '|' <e> ELLIPSIS name '|'") {
        Parser parser("{ |...args| }");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const parse::BlockNode* block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kBlock);
        block = reinterpret_cast<const parse::BlockNode*>(block->body->expr.get());

        REQUIRE(block->arguments != nullptr);
        REQUIRE(block->arguments->varArgsNameIndex);
        auto name = parser.lexer()->tokens()[block->arguments->varArgsNameIndex.value()];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("args") == 0);
        REQUIRE(block->arguments->varList);
        CHECK(block->arguments->varList->definitions == nullptr);
    }
}

// retval: <e> | '^' expr optsemi
TEST_CASE("Parser methbody") {
    SUBCASE("methbody: retval") {
        Parser parser("{ ^this }");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const parse::BlockNode* block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kBlock);
        block = reinterpret_cast<const parse::BlockNode*>(block->body->expr.get());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kReturn);
        auto retVal = reinterpret_cast<const parse::ReturnNode*>(block->body->expr.get());
        REQUIRE(retVal->valueExpr != nullptr);
        REQUIRE(retVal->valueExpr->nodeType == parse::NodeType::kName);
        auto nameNode = reinterpret_cast<const parse::NameNode*>(retVal->valueExpr.get());
        auto name = parser.lexer()->tokens()[nameNode->tokenIndex];
        CHECK(name.name == Token::kIdentifier);
        CHECK(name.range.compare("this") == 0);
    }

    SUBCASE("methbody: exprseq retval") {
        Parser parser("{ 1; 2; ^3; }");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const parse::BlockNode* block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kBlock);
        block = reinterpret_cast<const parse::BlockNode*>(block->body->expr.get());
        REQUIRE(block->body != nullptr);
        const auto exprSeq = block->body.get();

        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getInt32() == 1);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(literal->next.get());
        CHECK(literal->value.getInt32() == 2);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kReturn);
        auto retVal = reinterpret_cast<const parse::ReturnNode*>(literal->next.get());
        REQUIRE(retVal->valueExpr != nullptr);
        REQUIRE(retVal->valueExpr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(retVal->valueExpr.get());
        CHECK(literal->value.getInt32() == 3);
    }
}

// exprn: expr | exprn ';' expr
TEST_CASE("Parser exprseq") {
    SUBCASE("exprseq: exprn optsemi") {
        Parser parser("( x; y; z )");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kExprSeq);
        const parse::ExprSeqNode* exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(block->body.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kName);
        const parse::NameNode* nameNode = reinterpret_cast<const parse::NameNode*>(exprSeq->expr.get());
        auto name = parser.lexer()->tokens()[nameNode->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("x") == 0);

        REQUIRE(nameNode->next != nullptr);
        REQUIRE(nameNode->next->nodeType == parse::NodeType::kName);
        nameNode = reinterpret_cast<const parse::NameNode*>(nameNode->next.get());
        name = parser.lexer()->tokens()[nameNode->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("y") == 0);

        REQUIRE(nameNode->next != nullptr);
        REQUIRE(nameNode->next->nodeType == parse::NodeType::kName);
        nameNode = reinterpret_cast<const parse::NameNode*>(nameNode->next.get());
        name = parser.lexer()->tokens()[nameNode->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("z") == 0);
        CHECK(nameNode->next == nullptr);
    }
}

TEST_CASE("Parser msgsend") {
    SUBCASE("msgsend: name blocklist1") {
        Parser parser("bazoolie { false } { nil };");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const parse::BlockNode* block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kCall);
        auto call = reinterpret_cast<const parse::CallNode*>(block->body->expr.get());
        CHECK(!call->selectorImplied);
        CHECK_EQ(parser.lexer()->tokens()[call->tokenIndex].range.compare("bazoolie"), 0);
        CHECK(call->target == nullptr);
        CHECK(call->keywordArguments == nullptr);
        CHECK(call->next == nullptr);
        REQUIRE(call->arguments != nullptr);
        REQUIRE(call->arguments->nodeType == parse::NodeType::kBlock);

        block = reinterpret_cast<const parse::BlockNode*>(call->arguments.get());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kSlot);

        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(block->body->expr.get());
        CHECK(!literal->value.getBool());

        REQUIRE(block->next != nullptr);
        REQUIRE(block->next->nodeType == parse::NodeType::kBlock);
        block = reinterpret_cast<const parse::BlockNode*>(block->next.get());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(block->body->expr.get());
        CHECK(literal->value.isNil());
    }

    SUBCASE("msgsend: '(' binop2 ')' blocklist1") {
    }

    SUBCASE("msgsend: name '(' ')' blocklist1") {
    }

    SUBCASE("msgsend: name '(' arglist1 optkeyarglist ')' blocklist") {
    }

    SUBCASE("msgsend: '(' binop2 ')' '(' ')' blocklist1") {
    }

    SUBCASE("msgsend: '(' binop2 ')' '(' arglist1 optkeyarglist ')' blocklist") {
    }

    SUBCASE("msgsend: name '(' arglistv1 optkeyarglist ')'") {
    }

    SUBCASE("msgsend: '(' binop2 ')' '(' arglistv1 optkeyarglist ')'") {
    }

    SUBCASE("msgsend: classname '[' arrayelems ']'") {
    }

    SUBCASE("msgsend: classname blocklist1") {
        Parser parser("Routine{}");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kNew);
        auto newCall = reinterpret_cast<const parse::NewNode*>(block->body->expr.get());
        REQUIRE(newCall->arguments);
        REQUIRE(newCall->arguments->nodeType == parse::NodeType::kBlock);
        CHECK(newCall->keywordArguments == nullptr);
        REQUIRE(newCall->target);
        REQUIRE(newCall->target->nodeType == parse::NodeType::kName);
        CHECK(parser.lexer()->tokens()[newCall->target->tokenIndex].range.compare("Routine") == 0);
    }

    SUBCASE("msgsend: classname '(' ')' blocklist") {
        Parser parser("Dictionary()");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kNew);
        auto newNode = reinterpret_cast<const parse::NewNode*>(block->body->expr.get());
        CHECK(newNode->arguments == nullptr);
        CHECK(newNode->keywordArguments == nullptr);
        REQUIRE(newNode->target);
        REQUIRE(newNode->target->nodeType == parse::NodeType::kName);
        CHECK(parser.lexer()->tokens()[newNode->target->tokenIndex].range.compare("Dictionary") == 0);
    }

    SUBCASE("msgsend: classname '(' keyarglist1 optcomma ')' blocklist") {
        Parser parser("Trousers(blue: false)");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kNew);
        auto newNode = reinterpret_cast<const parse::NewNode*>(block->body->expr.get());
        CHECK(newNode->arguments == nullptr);
        REQUIRE(newNode->keywordArguments);
        REQUIRE(newNode->target);
        REQUIRE(newNode->target->nodeType == parse::NodeType::kName);
        CHECK(parser.lexer()->tokens()[newNode->target->tokenIndex].range.compare("Trousers") == 0);
    }

    SUBCASE("msgsend: classname '(' arglist1 optkeyarglist ')' blocklist") {
        Parser parser("SkipJack({ \"bazz\".postln; }, dt: 0.1);");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kNew);
        auto newNode = reinterpret_cast<const parse::NewNode*>(block->body->expr.get());
        REQUIRE(newNode->arguments);
        REQUIRE(newNode->keywordArguments);
        REQUIRE(newNode->target);
        REQUIRE(newNode->target->nodeType == parse::NodeType::kName);
        CHECK(parser.lexer()->tokens()[newNode->target->tokenIndex].range.compare("SkipJack") == 0);
    }

    SUBCASE("msgsend: classname '(' arglistv1 optkeyarglist ')'") {
    }

    SUBCASE("msgsend: expr '.' '(' ')' blocklist") {
    }

    SUBCASE("msgsend: expr '.' '(' keyarglist1 optcomma ')' blocklist") {
    }

    SUBCASE("msgsend: expr '.' name '(' keyarglist1 optcomma ')' blocklist") {
        Parser parser("SinOsc.ar(freq: 440, phase: 0, mul: 0.7,)");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kCall);
        auto call = reinterpret_cast<const parse::CallNode*>(block->body->expr.get());
        CHECK(!call->selectorImplied);
        CHECK_EQ(parser.lexer()->tokens()[call->tokenIndex].range.compare("ar"), 0);
        CHECK(call->arguments == nullptr);

        REQUIRE(call->target != nullptr);
        REQUIRE(call->target->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(call->target.get());
        auto nameToken = parser.lexer()->tokens()[name->tokenIndex];
        CHECK(nameToken.name == Token::kClassName);
        CHECK(nameToken.range.compare("SinOsc") == 0);

        REQUIRE(call->keywordArguments != nullptr);
        REQUIRE(call->keywordArguments->nodeType == parse::NodeType::kKeyValue);
        const parse::KeyValueNode* keyValue = reinterpret_cast<const parse::KeyValueNode*>(
                call->keywordArguments.get());
        nameToken = parser.lexer()->tokens()[keyValue->tokenIndex];
        CHECK(nameToken.name == Token::kKeyword);
        CHECK(nameToken.range.compare("freq") == 0);

        REQUIRE(keyValue->value != nullptr);
        REQUIRE(keyValue->value->nodeType == parse::NodeType::kExprSeq);
        const parse::ExprSeqNode* exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(keyValue->value.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);

        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getInt32() == 440);

        REQUIRE(keyValue->next != nullptr);
        REQUIRE(keyValue->next->nodeType == parse::NodeType::kKeyValue);
        keyValue = reinterpret_cast<const parse::KeyValueNode*>(keyValue->next.get());
        nameToken = parser.lexer()->tokens()[keyValue->tokenIndex];
        CHECK(nameToken.name == Token::kKeyword);
        CHECK(nameToken.range.compare("phase") == 0);
        REQUIRE(keyValue->value != nullptr);
        REQUIRE(keyValue->value->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(keyValue->value.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getInt32() == 0);

        REQUIRE(keyValue->next != nullptr);
        REQUIRE(keyValue->next->nodeType == parse::NodeType::kKeyValue);
        keyValue = reinterpret_cast<const parse::KeyValueNode*>(keyValue->next.get());
        nameToken = parser.lexer()->tokens()[keyValue->tokenIndex];
        CHECK(nameToken.name == Token::kKeyword);
        CHECK(nameToken.range.compare("mul") == 0);
        REQUIRE(keyValue->value != nullptr);
        REQUIRE(keyValue->value != nullptr);
        REQUIRE(keyValue->value->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(keyValue->value.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getFloat() == 0.7);
        CHECK(keyValue->next == nullptr);
    }

    SUBCASE("msgsend: expr '.' '(' arglist1 optkeyarglist ')' blocklist") {
        Parser parser("wakeup.(queue);");
        REQUIRE(parser.parse());
        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kCall);
        auto call = reinterpret_cast<const parse::CallNode*>(block->body->expr.get());
        CHECK(call->selectorImplied);
        CHECK(call->arguments != nullptr);
        REQUIRE(call->arguments->nodeType == parse::NodeType::kExprSeq);
        const parse::ExprSeqNode* exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(call->arguments.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kName);
        const auto name = reinterpret_cast<const parse::NameNode*>(exprSeq->expr.get());
        const auto& nameToken = parser.lexer()->tokens()[name->tokenIndex];
        CHECK(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("queue") == 0);
        CHECK(call->keywordArguments == nullptr);
    }

    SUBCASE("msgsend: expr '.' '(' arglistv1 optkeyarglist ')'") {
    }

    SUBCASE("msgsend: expr '.' name '(' ')' blocklist") {
        Parser parser("Array.new();");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kCall);
        auto call = reinterpret_cast<const parse::CallNode*>(block->body->expr.get());
        CHECK(!call->selectorImplied);
        CHECK_EQ(parser.lexer()->tokens()[call->tokenIndex].range.compare("new"), 0);
        CHECK(call->arguments == nullptr);
        CHECK(call->keywordArguments == nullptr);

        REQUIRE(call->target != nullptr);
        REQUIRE(call->target->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(call->target.get());
        auto nameToken = parser.lexer()->tokens()[name->tokenIndex];
        REQUIRE(nameToken.name == Token::kClassName);
        CHECK(nameToken.range.compare("Array") == 0);
    }

    SUBCASE("msgsend: expr '.' name '(' arglist1 optkeyarglist ')' blocklist") {
        Parser parser("this.method(x, y, z, a: 1, b:true, c:false)");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kCall);
        auto call = reinterpret_cast<const parse::CallNode*>(block->body->expr.get());
        CHECK(!call->selectorImplied);
        CHECK_EQ(parser.lexer()->tokens()[call->tokenIndex].range.compare("method"), 0);
        REQUIRE(call->target != nullptr);
        REQUIRE(call->target->nodeType == parse::NodeType::kName);
        const parse::NameNode* name = reinterpret_cast<const parse::NameNode*>(call->target.get());
        auto nameToken = parser.lexer()->tokens()[name->tokenIndex];
        CHECK(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("this") == 0);

        REQUIRE(call->arguments != nullptr);
        REQUIRE(call->arguments->nodeType == parse::NodeType::kExprSeq);
        const parse::ExprSeqNode* exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(call->arguments.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(exprSeq->expr.get());
        nameToken = parser.lexer()->tokens()[name->tokenIndex];
        CHECK(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("x") == 0);

        REQUIRE(exprSeq->next != nullptr);
        REQUIRE(exprSeq->next->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(exprSeq->next.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(exprSeq->expr.get());
        nameToken = parser.lexer()->tokens()[name->tokenIndex];
        CHECK(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("y") == 0);

        REQUIRE(exprSeq->next != nullptr);
        REQUIRE(exprSeq->next->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(exprSeq->next.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(exprSeq->expr.get());
        nameToken = parser.lexer()->tokens()[name->tokenIndex];
        CHECK(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("z") == 0);
        CHECK(exprSeq->next == nullptr);

        REQUIRE(call->keywordArguments != nullptr);
        REQUIRE(call->keywordArguments->nodeType == parse::NodeType::kKeyValue);
        const parse::KeyValueNode* keyValue = reinterpret_cast<const parse::KeyValueNode*>(
                call->keywordArguments.get());
        nameToken = parser.lexer()->tokens()[keyValue->tokenIndex];
        CHECK(nameToken.name == Token::kKeyword);
        CHECK(nameToken.range.compare("a") == 0);
        REQUIRE(keyValue->value != nullptr);
        REQUIRE(keyValue->value->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(keyValue->value.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getInt32() == 1);

        REQUIRE(keyValue->next != nullptr);
        REQUIRE(keyValue->next->nodeType == parse::NodeType::kKeyValue);
        keyValue = reinterpret_cast<const parse::KeyValueNode*>(keyValue->next.get());
        nameToken = parser.lexer()->tokens()[keyValue->tokenIndex];
        CHECK(nameToken.name == Token::kKeyword);
        CHECK(nameToken.range.compare("b") == 0);
        REQUIRE(keyValue->value != nullptr);
        REQUIRE(keyValue->value->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(keyValue->value.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getBool());

        REQUIRE(keyValue->next != nullptr);
        REQUIRE(keyValue->next->nodeType == parse::NodeType::kKeyValue);
        keyValue = reinterpret_cast<const parse::KeyValueNode*>(keyValue->next.get());
        nameToken = parser.lexer()->tokens()[keyValue->tokenIndex];
        CHECK(nameToken.name == Token::kKeyword);
        CHECK(nameToken.range.compare("c") == 0);
        REQUIRE(keyValue->value != nullptr);
        REQUIRE(keyValue->value->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(keyValue->value.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(!literal->value.getBool());
        CHECK(keyValue->next == nullptr);
    }

    SUBCASE("msgsend: expr '.' name '(' arglistv1 optkeyarglist ')'") {
    }

    SUBCASE("msgsend: expr '.' name blocklist") {
        Parser parser("4.neg");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kCall);
        auto call = reinterpret_cast<const parse::CallNode*>(block->body->expr.get());
        CHECK(!call->selectorImplied);
        CHECK_EQ(parser.lexer()->tokens()[call->tokenIndex].range.compare("neg"), 0);
        CHECK(call->arguments == nullptr);
        CHECK(call->keywordArguments == nullptr);

        REQUIRE(call->target != nullptr);
        REQUIRE(call->target->nodeType == parse::NodeType::kSlot);
        auto literal = reinterpret_cast<const parse::SlotNode*>(call->target.get());
        CHECK(literal->value.getInt32() == 4);
    }
}

TEST_CASE("Parser expr") {
    SUBCASE("expr: expr1") {
        Parser parser("\\g");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kSymbol);
    }

    SUBCASE("expr: valrangexd") {
    }

    SUBCASE("expr: valrangeassign") {
    }

    SUBCASE("expr: classname") {
        Parser parser("Object");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(block->body->expr.get());
        auto nameToken = parser.lexer()->tokens()[name->tokenIndex];
        REQUIRE(nameToken.name == Token::kClassName);
        CHECK(nameToken.range.compare("Object") == 0);
        CHECK(!name->isGlobal);
    }

    SUBCASE("expr: expr '.' '[' arglist1 ']'") {
    }

    SUBCASE("expr: '`' expr") {
    }

    SUBCASE("expr: expr binop2 adverb expr %prec binop") {
        Parser parser("a + b not: c");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kBinopCall);
        const parse::BinopCallNode* binop = reinterpret_cast<const parse::BinopCallNode*>(block->body->expr.get());
        auto nameToken = parser.lexer()->tokens()[binop->tokenIndex];
        REQUIRE(nameToken.name == Token::kKeyword);
        CHECK(nameToken.range.compare("not") == 0);

        REQUIRE(binop->rightHand != nullptr);
        REQUIRE(binop->rightHand->nodeType == parse::NodeType::kName);
        const parse::NameNode* name = reinterpret_cast<const parse::NameNode*>(binop->rightHand.get());
        nameToken = parser.lexer()->tokens()[name->tokenIndex];
        REQUIRE(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("c") == 0);

        REQUIRE(binop->leftHand != nullptr);
        REQUIRE(binop->leftHand->nodeType == parse::NodeType::kBinopCall);
        binop = reinterpret_cast<const parse::BinopCallNode*>(binop->leftHand.get());
        nameToken = parser.lexer()->tokens()[binop->tokenIndex];
        REQUIRE(nameToken.name == Token::kPlus);
        CHECK(nameToken.range.compare("+") == 0);
        REQUIRE(binop->leftHand != nullptr);
        REQUIRE(binop->leftHand->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(binop->leftHand.get());
        nameToken = parser.lexer()->tokens()[name->tokenIndex];
        REQUIRE(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("a") == 0);
        REQUIRE(binop->rightHand != nullptr);
        REQUIRE(binop->rightHand->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(binop->rightHand.get());
        nameToken = parser.lexer()->tokens()[name->tokenIndex];
        REQUIRE(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("b") == 0);
    }

    SUBCASE("expr: name '=' expr") {
        Parser parser("four = 4");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kAssign);
        auto assign = reinterpret_cast<const parse::AssignNode*>(block->body->expr.get());
        REQUIRE(assign->name != nullptr);
        auto name = parser.lexer()->tokens()[assign->name->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("four") == 0);
        CHECK(!assign->name->isGlobal);
        CHECK(assign->name->next == nullptr);
        REQUIRE(assign->value != nullptr);
        REQUIRE(assign->value->nodeType == parse::NodeType::kSlot);
        auto literal = reinterpret_cast<const parse::SlotNode*>(assign->value.get());
        CHECK(literal->value.getInt32() == 4);
    }

    SUBCASE("expr: '~' name '=' expr") {
        Parser parser("~globez = \"xyz\"");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kAssign);
        auto assign = reinterpret_cast<const parse::AssignNode*>(block->body->expr.get());
        REQUIRE(assign->name != nullptr);
        auto name = parser.lexer()->tokens()[assign->name->tokenIndex];
        REQUIRE(name.name == Token::kIdentifier);
        CHECK(name.range.compare("globez") == 0);
        CHECK(assign->name->isGlobal);
        CHECK(assign->name->next == nullptr);
        REQUIRE(assign->value != nullptr);
        REQUIRE(assign->value->nodeType == parse::NodeType::kString);
    }

    SUBCASE("expr: expr '.' name '=' expr") {
        Parser parser("~object.property = true");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kSetter);
        auto setter = reinterpret_cast<const parse::SetterNode*>(block->body->expr.get());
        auto nameToken = parser.lexer()->tokens()[setter->tokenIndex];
        REQUIRE(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("property") == 0);
        CHECK(setter->next == nullptr);

        REQUIRE(setter->target != nullptr);
        REQUIRE(setter->target->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(setter->target.get());
        nameToken = parser.lexer()->tokens()[name->tokenIndex];
        REQUIRE(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("object") == 0);
        CHECK(name->isGlobal);

        REQUIRE(setter->value != nullptr);
        REQUIRE(setter->value->nodeType == parse::NodeType::kSlot);
        auto literal = reinterpret_cast<const parse::SlotNode*>(setter->value.get());
        CHECK(literal->value.getBool());
    }

    SUBCASE("expr: name '(' arglist1 optkeyarglist ')' '=' expr") {
    }

    SUBCASE("expr: '#' mavars '=' expr") {
        // #a, b, c = [1, 2, 3];

    }

    SUBCASE("expr: expr1 '[' arglist1 ']' '=' expr") {
        Parser parser("bar[i] = \\foo");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kArrayWrite);
        auto arrayWrite = reinterpret_cast<const parse::ArrayWriteNode*>(block->body->expr.get());
        REQUIRE(arrayWrite->targetArray);
        REQUIRE(arrayWrite->targetArray->nodeType == parse::NodeType::kName);
        const parse::NameNode* name = reinterpret_cast<const parse::NameNode*>(arrayWrite->targetArray.get());
        CHECK(parser.lexer()->tokens()[name->tokenIndex].range.compare("bar") == 0);
        REQUIRE(arrayWrite->indexArgument);
        REQUIRE(arrayWrite->indexArgument->expr);
        REQUIRE(arrayWrite->indexArgument->expr->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(arrayWrite->indexArgument->expr.get());
        CHECK(parser.lexer()->tokens()[name->tokenIndex].range.compare("i") == 0);
        REQUIRE(arrayWrite->value);
        REQUIRE(arrayWrite->value->nodeType == parse::NodeType::kSymbol);
    }

    SUBCASE("expr: expr '.' '[' arglist1 ']' '=' expr") {
    }
}

TEST_CASE("Parser expr1") {
    SUBCASE("expr1: pushliteral") {
    }

    SUBCASE("expr1: blockliteral") {
    }

    SUBCASE("expr1: generator") {
    }

    SUBCASE("expr1: pushname") {
    }

    SUBCASE("expr1: curryarg") {
        Parser parser("string.removeAllSuchThat(_.isSpace);");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const parse::BlockNode* block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kCall);
        const parse::CallNode* call = reinterpret_cast<const parse::CallNode*>(block->body->expr.get());
        CHECK(!call->selectorImplied);
        CHECK_EQ(parser.lexer()->tokens()[call->tokenIndex].range.compare("removeAllSuchThat"), 0);
        REQUIRE(call->arguments);
        REQUIRE(call->arguments->nodeType == parse::NodeType::kExprSeq);
        const auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(call->arguments.get());
        REQUIRE(exprSeq->expr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kCall);
        call = reinterpret_cast<const parse::CallNode*>(exprSeq->expr.get());
        CHECK(!call->selectorImplied);
        CHECK_EQ(parser.lexer()->tokens()[call->tokenIndex].range.compare("isSpace"), 0);
        REQUIRE(call->target);
        REQUIRE(call->target->nodeType == parse::NodeType::kCurryArgument);
    }

    SUBCASE("expr1: msgsend") {
    }

    SUBCASE("expr1: '(' exprseq ')'") {
        Parser parser("{ arg bool; ^(this === bool).not }");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const parse::BlockNode* block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kBlock);
        block = reinterpret_cast<const parse::BlockNode*>(block->body->expr.get());

        CHECK(block->variables == nullptr);
        REQUIRE(block->arguments != nullptr);
        REQUIRE(block->arguments->varList != nullptr);
        REQUIRE(block->arguments->varList->definitions != nullptr);
        const auto defs = block->arguments->varList->definitions.get();
        auto nameToken = parser.lexer()->tokens()[defs->tokenIndex];
        REQUIRE(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("bool") == 0);
        CHECK(defs->initialValue == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kReturn);
        auto ret = reinterpret_cast<const parse::ReturnNode*>(block->body->expr.get());

        REQUIRE(ret->valueExpr != nullptr);
        REQUIRE(ret->valueExpr->nodeType == parse::NodeType::kCall);
        auto call = reinterpret_cast<const parse::CallNode*>(ret->valueExpr.get());
        CHECK(!call->selectorImplied);
        CHECK_EQ(parser.lexer()->tokens()[call->tokenIndex].range.compare("not"), 0);
        CHECK(call->arguments == nullptr);
        CHECK(call->keywordArguments == nullptr);

        REQUIRE(call->target->nodeType == parse::NodeType::kExprSeq);
        const auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(call->target.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kBinopCall);
        auto binop = reinterpret_cast<const parse::BinopCallNode*>(exprSeq->expr.get());
        nameToken = parser.lexer()->tokens()[binop->tokenIndex];
        REQUIRE(nameToken.name == Token::kBinop);
        CHECK(nameToken.range.compare("===") == 0);
        REQUIRE(binop->leftHand != nullptr);
        REQUIRE(binop->leftHand->nodeType == parse::NodeType::kName);
        const parse::NameNode* name = reinterpret_cast<const parse::NameNode*>(binop->leftHand.get());
        nameToken = parser.lexer()->tokens()[name->tokenIndex];
        REQUIRE(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("this") == 0);
        CHECK(!name->isGlobal);
        REQUIRE(binop->rightHand != nullptr);
        REQUIRE(binop->rightHand->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(binop->rightHand.get());
        CHECK(!name->isGlobal);
        nameToken = parser.lexer()->tokens()[name->tokenIndex];
        REQUIRE(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("bool") == 0);
    }

    SUBCASE("expr1: '~' name") {
        Parser parser("~z");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(block->body->expr.get());
        auto nameToken = parser.lexer()->tokens()[name->tokenIndex];
        CHECK(name->isGlobal);
        REQUIRE(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("z") == 0);
    }

    SUBCASE("expr1: '[' arrayelems ']'") {
        Parser parser("[a, b]");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kArray);
        const auto list = reinterpret_cast<const parse::ArrayNode*>(block->body->expr.get());
        REQUIRE(list->elements);
        REQUIRE(list->elements->nodeType == parse::NodeType::kExprSeq);
        const parse::ExprSeqNode* exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(list->elements.get());
        REQUIRE(exprSeq->expr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kName);
        const parse::NameNode* name = reinterpret_cast<const parse::NameNode*>(exprSeq->expr.get());
        auto nameToken = parser.lexer()->tokens()[name->tokenIndex];
        CHECK(nameToken.range.compare("a") == 0);
        REQUIRE(exprSeq->next);
        REQUIRE(exprSeq->next->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(exprSeq->next.get());
        REQUIRE(exprSeq->expr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(exprSeq->expr.get());
        nameToken = parser.lexer()->tokens()[name->tokenIndex];
        CHECK(nameToken.range.compare("b") == 0);
    }

    SUBCASE("expr1: '(' valrange2 ')'") {
    }

    SUBCASE("expr1: '(' ':' valrange3 ')'") {
    }

    SUBCASE("expr1: '(' dictslotlist ')'") {
        Parser parser("().call()");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kCall);
        auto call = reinterpret_cast<const parse::CallNode*>(block->body->expr.get());
        CHECK(!call->selectorImplied);
        CHECK_EQ(parser.lexer()->tokens()[call->tokenIndex].range.compare("call"), 0);
        REQUIRE(call->target);
        REQUIRE(call->target->nodeType == parse::NodeType::kEvent);
        auto dict = reinterpret_cast<const parse::EventNode*>(call->target.get());
        CHECK(dict->elements == nullptr);
        CHECK(call->arguments == nullptr);
        CHECK(call->keywordArguments == nullptr);
    }

    SUBCASE("expr1: pseudovar") {
    }

    SUBCASE("expr1: expr1 '[' arglist1 ']'") {
        Parser parser("text[0]");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kArrayRead);
        auto arrayRead = reinterpret_cast<const parse::ArrayReadNode*>(block->body->expr.get());
        REQUIRE(arrayRead->targetArray);
        REQUIRE(arrayRead->targetArray->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(arrayRead->targetArray.get());
        CHECK(parser.lexer()->tokens()[name->tokenIndex].range.compare("text") == 0);
        REQUIRE(arrayRead->indexArgument);
        REQUIRE(arrayRead->indexArgument->expr);
        REQUIRE(arrayRead->indexArgument->expr->nodeType == parse::NodeType::kSlot);
        auto literal = reinterpret_cast<const parse::SlotNode*>(arrayRead->indexArgument->expr.get());
        CHECK(literal->value == Slot::makeInt32(0));
    }

    SUBCASE("expr1: valrangex1") {
        Parser parser("target[4..]");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kCopySeries);
        const auto copySeries = reinterpret_cast<const parse::CopySeriesNode*>(block->body->expr.get());
        REQUIRE(copySeries->target);
        REQUIRE(copySeries->target->nodeType == parse::NodeType::kName);
        const auto name = reinterpret_cast<const parse::NameNode*>(copySeries->target.get());
        CHECK(parser.lexer()->tokens()[name->tokenIndex].range.compare("target") == 0);
        REQUIRE(copySeries->first);
        REQUIRE(copySeries->first->expr);
        REQUIRE(copySeries->first->expr->nodeType == parse::NodeType::kSlot);
        const auto literal = reinterpret_cast<const parse::SlotNode*>(copySeries->first->expr.get());
        CHECK(literal->value == Slot::makeInt32(4));
        CHECK(copySeries->last == nullptr);
    }
}

TEST_CASE("Parser valrangex1") {
    SUBCASE("valrangex1: expr1 '[' arglist1 DOTDOT ']'") {
        Parser parser("target[3,5..]");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kCopySeries);
        const auto copySeries = reinterpret_cast<const parse::CopySeriesNode*>(block->body->expr.get());
        REQUIRE(copySeries->target);
        REQUIRE(copySeries->target->nodeType == parse::NodeType::kName);
        const auto name = reinterpret_cast<const parse::NameNode*>(copySeries->target.get());
        CHECK(parser.lexer()->tokens()[name->tokenIndex].range.compare("target") == 0);
        REQUIRE(copySeries->first);
        REQUIRE(copySeries->first->expr);
        REQUIRE(copySeries->first->expr->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(copySeries->first->expr.get());
        CHECK(literal->value == Slot::makeInt32(3));
        REQUIRE(copySeries->second);
        REQUIRE(copySeries->second->nodeType == parse::NodeType::kExprSeq);
        const auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(copySeries->second.get());
        REQUIRE(exprSeq->expr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value == Slot::makeInt32(5));
        CHECK(copySeries->last == nullptr);
    }

    SUBCASE("valrangex1: expr1 '[' DOTDOT exprseq ']'") {
        Parser parser("~a[..~a.size - 3]");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kCopySeries);
        const auto copySeries = reinterpret_cast<const parse::CopySeriesNode*>(block->body->expr.get());
        REQUIRE(copySeries->target);
        REQUIRE(copySeries->target->nodeType == parse::NodeType::kName);
        const auto name = reinterpret_cast<const parse::NameNode*>(copySeries->target.get());
        CHECK(parser.lexer()->tokens()[name->tokenIndex].range.compare("a") == 0);
        CHECK(name->isGlobal);
        CHECK(copySeries->first == nullptr);
        REQUIRE(copySeries->last);
        REQUIRE(copySeries->last->expr);
        REQUIRE(copySeries->last->expr->nodeType == parse::NodeType::kBinopCall);
    }

    SUBCASE("valrangex1: expr1 '[' arglist1 DOTDOT exprseq ']'") {
        Parser parser("notes[a..z]");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kCopySeries);
        const auto copySeries = reinterpret_cast<const parse::CopySeriesNode*>(block->body->expr.get());
        REQUIRE(copySeries->target);
        REQUIRE(copySeries->target->nodeType == parse::NodeType::kName);
        const parse::NameNode* name = reinterpret_cast<const parse::NameNode*>(copySeries->target.get());
        CHECK(parser.lexer()->tokens()[name->tokenIndex].range.compare("notes") == 0);
        REQUIRE(copySeries->first);
        REQUIRE(copySeries->first->expr);
        REQUIRE(copySeries->first->expr->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(copySeries->first->expr.get());
        CHECK(parser.lexer()->tokens()[name->tokenIndex].range.compare("a") == 0);
        REQUIRE(copySeries->last);
        REQUIRE(copySeries->last->expr);
        REQUIRE(copySeries->last->expr->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(copySeries->last->expr.get());
        CHECK(parser.lexer()->tokens()[name->tokenIndex].range.compare("z") == 0);
    }
}

TEST_CASE("Parser valrange2") {
    SUBCASE("valrange2: exprseq DOTDOT") {
        // TODO: This will parse but is not a valid SC construction. Perhaps find a valid one?
        Parser parser("(4..)");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kSeries);
        auto series = reinterpret_cast<const parse::SeriesNode*>(block->body->expr.get());
        REQUIRE(series->start);
        REQUIRE(series->start->expr);
        REQUIRE(series->start->expr->nodeType == parse::NodeType::kSlot);
        auto literal = reinterpret_cast<const parse::SlotNode*>(series->start->expr.get());
        CHECK(literal->value == Slot::makeInt32(4));
        CHECK(series->step == nullptr);
        CHECK(series->last == nullptr);
    }

    SUBCASE("valrange2: DOTDOT exprseq") {
        Parser parser("(..c)");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kSeries);
        auto series = reinterpret_cast<const parse::SeriesNode*>(block->body->expr.get());
        CHECK(series->start == nullptr);
        CHECK(series->step == nullptr);
        REQUIRE(series->last);
        REQUIRE(series->last->expr);
        REQUIRE(series->last->expr->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(series->last->expr.get());
        CHECK(parser.lexer()->tokens()[name->tokenIndex].range.compare("c") == 0);
    }

    SUBCASE("valrange2: exprseq DOTDOT exprseq") {
        Parser parser("(0..this.instVarSize-1)");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kSeries);
        auto series = reinterpret_cast<const parse::SeriesNode*>(block->body->expr.get());
        REQUIRE(series->start);
        REQUIRE(series->start->expr);
        REQUIRE(series->start->expr->nodeType == parse::NodeType::kSlot);
        auto literal = reinterpret_cast<const parse::SlotNode*>(series->start->expr.get());
        CHECK(literal->value == Slot::makeInt32(0));
        CHECK(series->step == nullptr);
        REQUIRE(series->last);
        REQUIRE(series->last->expr);
        REQUIRE(series->last->expr->nodeType == parse::NodeType::kBinopCall);
    }

    SUBCASE("valrange2: exprseq ',' exprseq DOTDOT") {
    }

    SUBCASE("valrange2: exprseq ',' exprseq DOTDOT exprseq") {
        Parser parser("(1,3..99)");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kSeries);
        auto series = reinterpret_cast<const parse::SeriesNode*>(block->body->expr.get());
        REQUIRE(series->start);
        REQUIRE(series->start->expr);
        REQUIRE(series->start->expr->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(series->start->expr.get());
        CHECK(literal->value == Slot::makeInt32(1));
        REQUIRE(series->step);
        REQUIRE(series->step->expr);
        REQUIRE(series->step->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(series->step->expr.get());
        CHECK(literal->value == Slot::makeInt32(3));
        REQUIRE(series->last);
        REQUIRE(series->last->expr);
        REQUIRE(series->last->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(series->last->expr.get());
        CHECK(literal->value == Slot::makeInt32(99));
    }
}

TEST_CASE("Parser valrange3") {
    SUBCASE("valrange3: exprseq DOTDOT") {
    }

    SUBCASE("valrange3: DOTDOT exprseq") {
    }

    SUBCASE("valrange3: exprseq DOTDOT exprseq") {
    }

    SUBCASE("valrange3: exprseq ',' exprseq DOTDOT") {
    }

    SUBCASE("valrange3: exprseq ',' exprseq DOTDOT exprseq") {
    }
}

TEST_CASE("Parser literal") {
    SUBCASE("literal: '-' INTEGER") {
        Parser parser("- /*****/ 1");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kSlot);
        auto literal = reinterpret_cast<const parse::SlotNode*>(block->body->expr.get());
        CHECK(literal->value.getInt32() == -1);
    }
}

TEST_CASE("Parser arrayelems") {
    SUBCASE("arrayelems: <e>") {
        Parser parser("[ ]");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kArray);
        auto array = reinterpret_cast<const parse::ArrayNode*>(block->body->expr.get());
        CHECK(array->elements == nullptr);
    }

    SUBCASE("arrayelems: arrayelems1 optcomma") {
        Parser parser("[1,-2,]");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kArray);
        auto array = reinterpret_cast<const parse::ArrayNode*>(block->body->expr.get());

        REQUIRE(array->elements != nullptr);
        REQUIRE(array->elements->nodeType == parse::NodeType::kExprSeq);
        const parse::ExprSeqNode* exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(array->elements.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getInt32() == 1);
        CHECK(literal->next == nullptr);

        REQUIRE(exprSeq->next != nullptr);
        REQUIRE(exprSeq->next->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(exprSeq->next.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getInt32() == -2);
    }
}

TEST_CASE("Parser arrayelems1") {
    SUBCASE("arrayelems1: exprseq") {
        Parser parser("[ 3; a; nil, ]");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kArray);
        auto array = reinterpret_cast<const parse::ArrayNode*>(block->body->expr.get());

        REQUIRE(array->elements != nullptr);
        REQUIRE(array->elements->nodeType == parse::NodeType::kExprSeq);
        auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(array->elements.get());

        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getInt32() == 3);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(literal->next.get());
        auto nameToken = parser.lexer()->tokens()[name->tokenIndex];
        REQUIRE(nameToken.name == Token::kIdentifier);
        CHECK(nameToken.range.compare("a") == 0);

        REQUIRE(name->next != nullptr);
        REQUIRE(name->next->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(name->next.get());
        CHECK(literal->value.isNil());
        CHECK(literal->next == nullptr);
    }

    SUBCASE("arrayelems1: exprseq ':' exprseq") {
        Parser parser("[ 1;2: 3;4 ]");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);

        REQUIRE(block->body->expr->nodeType == parse::NodeType::kArray);
        auto array = reinterpret_cast<const parse::ArrayNode*>(block->body->expr.get());

        REQUIRE(array->elements != nullptr);
        REQUIRE(array->elements->nodeType == parse::NodeType::kExprSeq);
        const parse::ExprSeqNode* exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(array->elements.get());

        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getInt32() == 1);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(literal->next.get());
        CHECK(literal->value.getInt32() == 2);

        REQUIRE(exprSeq->next != nullptr);
        REQUIRE(exprSeq->next->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(exprSeq->next.get());

        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getInt32() == 3);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(literal->next.get());
        CHECK(literal->value.getInt32() == 4);
    }

    SUBCASE("keybinop exprseq") {
        Parser parser("[freq: 440,]");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kArray);
        auto array = reinterpret_cast<const parse::ArrayNode*>(block->body->expr.get());

        REQUIRE(array->elements != nullptr);
        REQUIRE(array->elements->nodeType == parse::NodeType::kExprSeq);
        const parse::ExprSeqNode* exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(array->elements.get());
        REQUIRE(exprSeq->expr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSymbol);

        REQUIRE(exprSeq->next != nullptr);
        REQUIRE(exprSeq->next->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(exprSeq->next.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        const auto literal = reinterpret_cast<const parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value.getInt32() == 440);
        CHECK(literal->next == nullptr);
    }

    SUBCASE("arrayelems1 ',' exprseq") {
    }

    SUBCASE("arrayelems1 ',' keybinop exprseq") {
    }

    SUBCASE("arrayelems1 ',' exprseq ':' exprseq") {
    }
}

TEST_CASE("Parser if") {
    SUBCASE("if '(' exprseq ',' block ',' block optcomma')'") {
        Parser parser("if ([true, false].choose, { \"true\".postln; }, { \"false\".postln; }, );");
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->expr != nullptr);

        REQUIRE(block->body->expr->nodeType == parse::NodeType::kIf);
        const auto ifNode = reinterpret_cast<const parse::IfNode*>(block->body->expr.get());

        // [true, false].choose
        REQUIRE(ifNode->condition != nullptr);
        REQUIRE(ifNode->condition->expr != nullptr);
        REQUIRE(ifNode->condition->expr->nodeType == parse::NodeType::kCall);
        const parse::CallNode* call = reinterpret_cast<parse::CallNode*>(ifNode->condition->expr.get());
        CHECK(!call->selectorImplied);
        CHECK_EQ(parser.lexer()->tokens()[call->tokenIndex].range.compare("choose"), 0);
        REQUIRE(call->target != nullptr);
        REQUIRE(call->target->nodeType == parse::NodeType::kArray);
        const auto dynList = reinterpret_cast<const parse::ArrayNode*>(call->target.get());
        REQUIRE(dynList->elements != nullptr);
        REQUIRE(dynList->elements->nodeType == parse::NodeType::kExprSeq);
        const parse::ExprSeqNode* exprSeq = reinterpret_cast<parse::ExprSeqNode*>(dynList->elements.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value == Slot::makeBool(true));
        REQUIRE(exprSeq->next != nullptr);
        REQUIRE(exprSeq->next->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<parse::ExprSeqNode*>(exprSeq->next.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<parse::SlotNode*>(exprSeq->expr.get());
        CHECK(literal->value == Slot::makeBool(false));

        // { "true".postln }
        REQUIRE(ifNode->trueBlock);
        REQUIRE(ifNode->trueBlock->body);
        REQUIRE(ifNode->trueBlock->body->expr);
        REQUIRE(ifNode->trueBlock->body->expr->nodeType == parse::NodeType::kCall);
        call = reinterpret_cast<const parse::CallNode*>(ifNode->trueBlock->body->expr.get());
        CHECK(!call->selectorImplied);
        CHECK_EQ(parser.lexer()->tokens()[call->tokenIndex].range.compare("postln"), 0);
        REQUIRE(call->target);
        REQUIRE(call->target->nodeType == parse::NodeType::kString);
        auto token = parser.lexer()->tokens()[call->target->tokenIndex];
        CHECK(token.range.compare("true") == 0);
        CHECK(literal->next == nullptr);

        // { "false".postln }
        REQUIRE(ifNode->falseBlock);
        REQUIRE(ifNode->falseBlock->body);
        REQUIRE(ifNode->falseBlock->body->expr);
        REQUIRE(ifNode->falseBlock->body->expr->nodeType == parse::NodeType::kCall);
        call = reinterpret_cast<const parse::CallNode*>(ifNode->falseBlock->body->expr.get());
        CHECK(!call->selectorImplied);
        CHECK_EQ(parser.lexer()->tokens()[call->tokenIndex].range.compare("postln"), 0);
        REQUIRE(call->target);
        REQUIRE(call->target->nodeType == parse::NodeType::kString);
        token = parser.lexer()->tokens()[call->target->tokenIndex];
        CHECK(token.range.compare("false") == 0);
        CHECK(literal->next == nullptr);
    }

    SUBCASE("if '(' exprseq ',' block optcomma ')'") {
        Parser parser("if(x,{y},);");
        REQUIRE(parser.parse());

        REQUIRE(parser.root());
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kIf);
        const auto ifNode = reinterpret_cast<const parse::IfNode*>(block->body->expr.get());

        // x
        REQUIRE(ifNode->condition);
        REQUIRE(ifNode->condition->expr);
        REQUIRE(ifNode->condition->expr->nodeType == parse::NodeType::kName);
        const parse::NameNode* name = reinterpret_cast<const parse::NameNode*>(ifNode->condition->expr.get());
        auto token = parser.lexer()->tokens()[name->tokenIndex];
        CHECK(token.range.compare("x") == 0);

        // {y}
        REQUIRE(ifNode->trueBlock);
        REQUIRE(ifNode->trueBlock->body);
        REQUIRE(ifNode->trueBlock->body->expr);
        REQUIRE(ifNode->trueBlock->body->expr->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(ifNode->trueBlock->body->expr.get());
        token = parser.lexer()->tokens()[name->tokenIndex];
        CHECK(token.range.compare("y") == 0);

        CHECK(ifNode->falseBlock == nullptr);
        CHECK(ifNode->next == nullptr);
    }

    SUBCASE("expr.if '(' block ',' block optcomma ')'") {
        Parser parser("(x % 2).if({\\odd},{\\even});");
        REQUIRE(parser.parse());

        REQUIRE(parser.root());
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kIf);
        const auto ifNode = reinterpret_cast<const parse::IfNode*>(block->body->expr.get());

        // (x % 2)
        REQUIRE(ifNode->condition);
        REQUIRE(ifNode->condition->expr);
        REQUIRE(ifNode->condition->expr->nodeType == parse::NodeType::kExprSeq);
        const auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(ifNode->condition->expr.get());
        REQUIRE(exprSeq->expr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kBinopCall);
        const auto binop = reinterpret_cast<const parse::BinopCallNode*>(exprSeq->expr.get());
        auto token = parser.lexer()->tokens()[binop->tokenIndex];
        CHECK(token.range.compare("%") == 0);
        REQUIRE(binop->leftHand);
        CHECK(binop->leftHand->nodeType == parse::NodeType::kName);
        token = parser.lexer()->tokens()[binop->leftHand->tokenIndex];
        CHECK(token.range.compare("x") == 0);
        REQUIRE(binop->rightHand);
        CHECK(binop->rightHand->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(binop->rightHand.get());
        CHECK(literal->value == Slot::makeInt32(2));

        // {\odd}
        REQUIRE(ifNode->trueBlock);
        REQUIRE(ifNode->trueBlock->body);
        REQUIRE(ifNode->trueBlock->body->expr);
        REQUIRE(ifNode->trueBlock->body->expr->nodeType == parse::NodeType::kSymbol);
        token = parser.lexer()->tokens()[ifNode->trueBlock->body->expr->tokenIndex];
        CHECK(token.range.compare("odd") == 0);

        // {\even}
        REQUIRE(ifNode->falseBlock);
        REQUIRE(ifNode->falseBlock->body);
        REQUIRE(ifNode->falseBlock->body->expr);
        REQUIRE(ifNode->falseBlock->body->expr->nodeType == parse::NodeType::kSymbol);
        token = parser.lexer()->tokens()[ifNode->falseBlock->body->expr->tokenIndex];
        CHECK(token.range.compare("even") == 0);
    }

    SUBCASE("expr.if '(' block optcomma ')'") {
        Parser parser("true.if({-23},)");
        REQUIRE(parser.parse());

        REQUIRE(parser.root());
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kIf);
        const auto ifNode = reinterpret_cast<const parse::IfNode*>(block->body->expr.get());

        // true
        REQUIRE(ifNode->condition);
        REQUIRE(ifNode->condition->expr);
        REQUIRE(ifNode->condition->expr->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(ifNode->condition->expr.get());
        CHECK(literal->value == Slot::makeBool(true));

        // {-23}
        REQUIRE(ifNode->trueBlock);
        REQUIRE(ifNode->trueBlock->body);
        REQUIRE(ifNode->trueBlock->body->expr);
        REQUIRE(ifNode->trueBlock->body->expr->nodeType == parse::NodeType::kSlot);
        literal = reinterpret_cast<const parse::SlotNode*>(ifNode->trueBlock->body->expr.get());
        CHECK(literal->value == Slot::makeInt32(-23));
    }

    SUBCASE("if '(' exprseq ')' block optblock") {
        Parser parser("if(this.findMethod(methodName).isNil) { ^nil };");
        REQUIRE(parser.parse());

        REQUIRE(parser.root());
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kIf);
        const auto ifNode = reinterpret_cast<const parse::IfNode*>(block->body->expr.get());
        REQUIRE(ifNode->condition);
        REQUIRE(ifNode->condition->expr);

        // { ^nil }
        REQUIRE(ifNode->trueBlock);
        REQUIRE(ifNode->trueBlock->body);
        REQUIRE(ifNode->trueBlock->body->expr);
        REQUIRE(ifNode->trueBlock->body->expr->nodeType == parse::NodeType::kReturn);
        auto retNode = reinterpret_cast<const parse::ReturnNode*>(ifNode->trueBlock->body->expr.get());
        REQUIRE(retNode->valueExpr);
        REQUIRE(retNode->valueExpr->nodeType == parse::NodeType::kSlot);
        const parse::SlotNode* literal = reinterpret_cast<const parse::SlotNode*>(retNode->valueExpr.get());
        CHECK(literal->value == Slot::makeNil());

        CHECK(ifNode->falseBlock == nullptr);
    }
}

TEST_CASE("Parser while") {
    SUBCASE("while '(' block optcomma optblock ')'") {
        Parser parser("while ({true});");
        REQUIRE(parser.parse());
        REQUIRE(parser.root());
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kWhile);
        const auto whileNode = reinterpret_cast<const parse::WhileNode*>(block->body->expr.get());

        // {true}
        REQUIRE(whileNode->blocks);
        REQUIRE(whileNode->blocks->body);
        REQUIRE(whileNode->blocks->body->expr);
        REQUIRE_EQ(whileNode->blocks->body->expr->nodeType, parse::NodeType::kSlot);
        const auto literal = reinterpret_cast<const parse::SlotNode*>(whileNode->blocks->body->expr.get());
        CHECK_EQ(literal->value, Slot::makeBool(true));
        CHECK_EQ(literal->next, nullptr);

        CHECK_EQ(whileNode->blocks->next, nullptr);
    }

    SUBCASE("while block optcomma block") {
        Parser parser("while { counter < 5 } { this.doIt() }");
        REQUIRE(parser.parse());
        REQUIRE(parser.root());
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        REQUIRE(block->body);
        REQUIRE(block->body->expr);
        REQUIRE(block->body->expr->nodeType == parse::NodeType::kWhile);
        const auto whileNode = reinterpret_cast<const parse::WhileNode*>(block->body->expr.get());

        // { counter < 5 }
        REQUIRE(whileNode->blocks);
        REQUIRE(whileNode->blocks->body);
        REQUIRE(whileNode->blocks->body->expr);
        REQUIRE_EQ(whileNode->blocks->body->expr->nodeType, parse::NodeType::kBinopCall);

        // { this.doIt() }
        REQUIRE(whileNode->blocks->next);
        REQUIRE_EQ(whileNode->blocks->next->nodeType, parse::NodeType::kBlock);
        const auto repeatBlock = reinterpret_cast<const parse::BlockNode*>(whileNode->blocks->next.get());
        REQUIRE(repeatBlock->body);
        REQUIRE(repeatBlock->body->expr);
        REQUIRE_EQ(repeatBlock->body->expr->nodeType, parse::NodeType::kCall);
    }
}

} // namespace hadron
