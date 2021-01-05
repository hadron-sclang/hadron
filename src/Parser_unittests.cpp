#include "Parser.hpp"

#include "ErrorReporter.hpp"

#include "doctest/doctest.h"
#include "spdlog/spdlog.h"

namespace hadron {

// root: classes | classextensions | cmdlinecode
TEST_CASE("Parser root") {
    SUBCASE("root: <e>") {
        Parser parser("", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        CHECK(parser.root()->nodeType == parse::NodeType::kEmpty);
        CHECK(parser.root()->tokenIndex == 0);
        CHECK(parser.root()->next.get() == nullptr);
        CHECK(parser.root()->tail == parser.root());
    }

    SUBCASE("root: classes") {
        Parser parser("A { } B { }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root()->next.get());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("A") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->variables == nullptr);
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->next != nullptr);
        REQUIRE(classNode->next->nodeType == parse::NodeType::kClass);
        classNode = reinterpret_cast<const parse::ClassNode*>(classNode->next.get());
        CHECK(classNode->className.compare("B") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->variables == nullptr);
        CHECK(classNode->methods == nullptr);
    }

    SUBCASE("root: classextensions") {
        Parser parser("+ A { } + B { }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClassExt);
        CHECK(parser.root()->tail == parser.root()->next.get());
        auto classExt = reinterpret_cast<const parse::ClassExtNode*>(parser.root());
        CHECK(classExt->className.compare("A") == 0);
        CHECK(classExt->methods == nullptr);

        REQUIRE(classExt->next != nullptr);
        REQUIRE(classExt->next->nodeType == parse::NodeType::kClassExt);
        classExt = reinterpret_cast<const parse::ClassExtNode*>(classExt->next.get());
        CHECK(classExt->className.compare("B") == 0);
        CHECK(classExt->methods == nullptr);
    }

    SUBCASE("root: cmdlinecode") {
        Parser parser("42", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.root()->next == nullptr);
        CHECK(parser.root()->tail == parser.root());
        auto literal = reinterpret_cast<const parse::LiteralNode*>(parser.root());
        REQUIRE(literal->tokenIndex == 0);
        CHECK(parser.tokens()[literal->tokenIndex].type == Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[literal->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[literal->tokenIndex].value.asInteger() == 42);
    }
}

// classdef: classname superclass '{' classvardecls methods '}'
//         | classname '[' optname ']' superclass '{' classvardecls methods '}'
TEST_CASE("Parser classdef") {
    SUBCASE("classdef: classname superclass '{' classvardecls methods '}'") {
        Parser parser("A : B { var x; a { } }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->next == nullptr);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("A") == 0);
        CHECK(classNode->superClassName.compare("B") == 0);
        CHECK(classNode->optionalName.empty());

        REQUIRE(classNode->variables);
        REQUIRE(classNode->variables->definitions);
        CHECK(classNode->variables->definitions->varName.compare("x") == 0);
        CHECK(classNode->variables->definitions->initialValue == nullptr);
        CHECK(classNode->variables->definitions->next == nullptr);

        REQUIRE(classNode->methods);
        CHECK(classNode->methods->methodName.compare("a") == 0);
        CHECK(!classNode->methods->isClassMethod);
        CHECK(classNode->methods->arguments == nullptr);
        CHECK(classNode->methods->variables == nullptr);
        CHECK(classNode->methods->body == nullptr);
        CHECK(classNode->methods->next == nullptr);
    }

    SUBCASE("classdef: classname '[' optname ']' superclass '{' classvardecls methods '}'") {
        Parser parser("Sub [ opt ] : Super { const c = 5; *meth { } }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->next == nullptr);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("Sub") == 0);
        CHECK(classNode->optionalName.compare("opt") == 0);
        CHECK(classNode->superClassName.compare("Super") == 0);

        REQUIRE(classNode->variables);
        REQUIRE(classNode->variables->definitions);
        CHECK(classNode->variables->definitions->varName.compare("c") == 0);
        CHECK(classNode->variables->next == nullptr);

        REQUIRE(classNode->variables->definitions->initialValue);
        REQUIRE(classNode->variables->definitions->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[classNode->variables->definitions->initialValue->tokenIndex].type ==
                Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[classNode->variables->definitions->initialValue->tokenIndex].value.type() ==
                TypedValue::Type::kInteger);
        CHECK(parser.tokens()[classNode->variables->definitions->initialValue->tokenIndex].value.asInteger() == 5);
        CHECK(classNode->variables->definitions->next == nullptr);

        REQUIRE(classNode->methods);
        CHECK(classNode->methods->methodName.compare("meth") == 0);
        CHECK(classNode->methods->isClassMethod);
        CHECK(classNode->methods->arguments == nullptr);
        CHECK(classNode->methods->variables == nullptr);
        CHECK(classNode->methods->body == nullptr);
        CHECK(classNode->methods->next == nullptr);
    }
}

// classextension: '+' classname '{' methods '}'
TEST_CASE("Parser classextension") {
    SUBCASE("classextension: '+' classname '{' methods '}'") {
        Parser parser("+ Cls { *classMethod {} method {} }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClassExt);
        CHECK(parser.root()->next == nullptr);
        auto classExt = reinterpret_cast<const parse::ClassExtNode*>(parser.root());
        CHECK(classExt->className.compare("Cls") == 0);

        REQUIRE(classExt->methods != nullptr);
        CHECK(classExt->methods->methodName.compare("classMethod") == 0);
        CHECK(classExt->methods->isClassMethod);
        CHECK(classExt->methods->arguments == nullptr);
        CHECK(classExt->methods->variables == nullptr);
        CHECK(classExt->methods->body == nullptr);

        REQUIRE(classExt->methods->next != nullptr);
        REQUIRE(classExt->methods->next->nodeType == parse::NodeType::kMethod);
        auto method = reinterpret_cast<const parse::MethodNode*>(classExt->methods->next.get());
        CHECK(method->methodName.compare("method") == 0);
        CHECK(!method->isClassMethod);
        CHECK(method->arguments == nullptr);
        CHECK(method->variables == nullptr);
        CHECK(method->body == nullptr);
        CHECK(method->next == nullptr);
    }
}

// cmdlinecode: '(' funcvardecls1 funcbody ')'
//            | funcvardecls1 funcbody
//            | funcbody
TEST_CASE("Parser cmdlinecode") {
    SUBCASE("cmdlinecode: '(' funcvardecls1 funcbody ')'") {
        Parser parser("( var a; 0xa )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        CHECK(block->variables->definitions->varName.compare("a") == 0);
        CHECK(block->variables->definitions->initialValue == nullptr);
        CHECK(block->variables->definitions->next == nullptr);
        CHECK(block->variables->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[block->body->tokenIndex].type == Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[block->body->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[block->body->tokenIndex].value.asInteger() == 0xa);
        CHECK(block->body->next == nullptr);
    }

    SUBCASE("cmdlinecode: funcvardecls1 funcbody") {
        Parser parser("var x = \\ex; x", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        CHECK(block->variables->definitions->varName.compare("x") == 0);
        CHECK(block->variables->definitions->next == nullptr);
        CHECK(block->variables->next == nullptr);

        REQUIRE(block->variables->definitions->initialValue != nullptr);
        REQUIRE(block->variables->definitions->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[block->variables->definitions->initialValue->tokenIndex].type ==
                Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[block->variables->definitions->initialValue->tokenIndex].value.type() ==
                TypedValue::Type::kSymbol);
        CHECK(block->variables->definitions->initialValue->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(block->body.get());
        CHECK(name->name.compare("x") == 0);
        CHECK(name->next == nullptr);
    }

    SUBCASE("cmdlinecode: funcbody") {
        Parser parser("\"string\"", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.root()->next == nullptr);
        REQUIRE(parser.root()->tokenIndex == 0);
        CHECK(parser.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[0].value.type() == TypedValue::Type::kString);
    }
}

// classvardecls: <e> | classvardecls classvardecl
TEST_CASE("Parser classvardecls") {
    SUBCASE("classvardecls: <e>") {
        Parser parser("A { }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("A") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->variables == nullptr);
        CHECK(classNode->methods == nullptr);
    }

    SUBCASE("classvardecls: classvardecls classvardecl") {
        Parser parser("C { classvar a, b, c; var d, e, f; const g = 1, h = 2, i = 3; }",
                std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("C") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kClassVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("a") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("b") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("c") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);

        REQUIRE(varList->next != nullptr);
        REQUIRE(varList->next->nodeType == parse::NodeType::kVarList);
        varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kVar);

        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("d") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("e") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("f") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);

        REQUIRE(varList->next != nullptr);
        REQUIRE(varList->next->nodeType == parse::NodeType::kVarList);
        varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kConst);

        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("g") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asInteger() == 1);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("h") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asInteger() == 2);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("i") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asInteger() == 3);

        CHECK(varDef->next == nullptr);
        CHECK(varList->next == nullptr);
    }
}

// classvardecl: CLASSVAR rwslotdeflist ';'
//             | VAR rwslotdeflist ';'
//             | SC_CONST constdeflist ';'
TEST_CASE("Parser classvardecl") {
    SUBCASE("classvardecl: CLASSVAR rwslotdeflist ';'") {
        Parser parser("X { classvar <> a, > b, < c; }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("X") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kClassVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("a") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("b") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(!varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("c") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
    }

    SUBCASE("classvardecl: VAR rwslotdeflist ';'") {
        Parser parser("Y { var < d1, <> e2, > f3; }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("Y") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("d1") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("e2") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("f3") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
        CHECK(!varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);
    }

    SUBCASE("classvardecl: SC_CONST constdeflist ';'") {
        Parser parser("Z { const bogon = 42, <  red5 = \"goin' in\"; }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("Z") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kConst);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("bogon") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asInteger() == 42);
        CHECK(!varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("red5") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kString);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
    }
}

TEST_CASE("Parser methods") {
    SUBCASE("methods: <e>") {
        Parser parser("Zed { }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("Zed") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->methods == nullptr);
    }

    SUBCASE("methods: methods methoddef") {
        Parser parser("Multi { m { } ++ { } *x { } * * { } }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("Multi") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());

        REQUIRE(classNode->methods != nullptr);
        const parse::MethodNode* method = classNode->methods.get();
        CHECK(method->methodName.compare("m") == 0);
        CHECK(!method->isClassMethod);
        CHECK(method->arguments == nullptr);
        CHECK(method->variables == nullptr);
        CHECK(method->body == nullptr);

        REQUIRE(method->next != nullptr);
        REQUIRE(method->next->nodeType == parse::NodeType::kMethod);
        method = reinterpret_cast<const parse::MethodNode*>(method->next.get());
        CHECK(method->methodName.compare("++") == 0);
        CHECK(!method->isClassMethod);
        CHECK(method->arguments == nullptr);
        CHECK(method->variables == nullptr);
        CHECK(method->body == nullptr);

        REQUIRE(method->next != nullptr);
        REQUIRE(method->next->nodeType == parse::NodeType::kMethod);
        method = reinterpret_cast<const parse::MethodNode*>(method->next.get());
        CHECK(method->methodName.compare("x") == 0);
        CHECK(method->isClassMethod);
        CHECK(method->arguments == nullptr);
        CHECK(method->variables == nullptr);
        CHECK(method->body == nullptr);

        REQUIRE(method->next != nullptr);
        REQUIRE(method->next->nodeType == parse::NodeType::kMethod);
        method = reinterpret_cast<const parse::MethodNode*>(method->next.get());
        // This is an interesting parse, requiring a space between the class method indicator '*' and the binop '*'.
        // If the token is "**" that is parsed as a object binop method named "**".
        CHECK(method->methodName.compare("*") == 0);
        CHECK(method->isClassMethod);
        CHECK(method->arguments == nullptr);
        CHECK(method->variables == nullptr);
        CHECK(method->body == nullptr);
        CHECK(method->next == nullptr);
    }
}

TEST_CASE("Parser methoddef") {
    SUBCASE("methoddef: name '{' argdecls funcvardecls primitive methbody '}'") {
        Parser parser("W { m1 { |z| var c = z; _Prim; c; } }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("W") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());

        REQUIRE(classNode->methods != nullptr);
        const parse::MethodNode* method = classNode->methods.get();
        CHECK(method->methodName.compare("m1") == 0);
        CHECK(!method->isClassMethod);
        CHECK(method->primitive.compare("_Prim") == 0);

        REQUIRE(method->arguments != nullptr);
        auto argList = method->arguments.get();
        REQUIRE(argList->varList != nullptr);
        REQUIRE(argList->varList->definitions != nullptr);
        CHECK(argList->varList->definitions->varName.compare("z") == 0);
        CHECK(argList->varList->definitions->initialValue == nullptr);
        CHECK(argList->varList->definitions->next == nullptr);

        REQUIRE(method->variables != nullptr);
        auto varList = method->variables.get();
        REQUIRE(varList->definitions != nullptr);
        CHECK(varList->definitions->varName.compare("c") == 0);
        REQUIRE(varList->definitions->initialValue != nullptr);
        REQUIRE(varList->definitions->initialValue->nodeType == parse::NodeType::kName);
        auto nameNode = reinterpret_cast<const parse::NameNode*>(varList->definitions->initialValue.get());
        CHECK(nameNode->name.compare("z") == 0);

        REQUIRE(method->body != nullptr);
        REQUIRE(method->body->nodeType == parse::NodeType::kName);
        nameNode = reinterpret_cast<const parse::NameNode*>(method->body.get());
        CHECK(nameNode->name.compare("c") == 0);

        CHECK(classNode->next == nullptr);
    }

    SUBCASE("methoddef: binop '{' argdecls funcvardecls primitive methbody '}'") {
        Parser parser("Kz { +/+ { arg b, c; var m, n; _Thunk 17; } }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("Kz") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());

        REQUIRE(classNode->methods != nullptr);
        const parse::MethodNode* method = classNode->methods.get();
        CHECK(method->methodName.compare("+/+") == 0);
        CHECK(!method->isClassMethod);
        CHECK(method->primitive.compare("_Thunk") == 0);

        REQUIRE(method->arguments != nullptr);
        auto argList = method->arguments.get();
        REQUIRE(argList->varList != nullptr);
        REQUIRE(argList->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = argList->varList->definitions.get();
        CHECK(varDef->varName.compare("b") == 0);
        CHECK(varDef->initialValue == nullptr);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("c") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);

        REQUIRE(method->variables != nullptr);
        auto varList = method->variables.get();
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("m") == 0);
        CHECK(varDef->initialValue == nullptr);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("n") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);

        REQUIRE(method->body != nullptr);
        CHECK(method->body->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[method->body->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[method->body->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[method->body->tokenIndex].value.asInteger() == 17);

        CHECK(classNode->next == nullptr);
    }

    SUBCASE("methoddef: '*' name '{' argdecls funcvardecls primitive methbody '}'") {
        Parser parser("Mx { *clsMeth { |m=5, n=7| var k = 0; var z = \\sym; _X ^\\k } }",
                std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("Mx") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());

        REQUIRE(classNode->methods != nullptr);
        const parse::MethodNode* method = classNode->methods.get();
        CHECK(method->methodName.compare("clsMeth") == 0);
        CHECK(method->isClassMethod);
        CHECK(method->primitive.compare("_X") == 0);

        REQUIRE(method->arguments != nullptr);
        auto argList = method->arguments.get();
        REQUIRE(argList->varList != nullptr);
        REQUIRE(argList->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = argList->varList->definitions.get();
        CHECK(varDef->varName.compare("m") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        CHECK(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asInteger() == 5);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("n") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        CHECK(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asInteger() == 7);
        CHECK(varDef->next == nullptr);

        REQUIRE(method->variables != nullptr);
        const parse::VarListNode* varList = method->variables.get();
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("k") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        CHECK(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asInteger() == 0);
        CHECK(varDef->next == nullptr);
        REQUIRE(varList->next != nullptr);
        REQUIRE(varList->next->nodeType == parse::NodeType::kVarList);
        varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("z") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        CHECK(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kSymbol);
        CHECK(varDef->next == nullptr);
        CHECK(varList->next == nullptr);

        REQUIRE(method->body != nullptr);
        REQUIRE(method->body->nodeType == parse::NodeType::kReturn);
        auto retNode = reinterpret_cast<const parse::ReturnNode*>(method->body.get());
        REQUIRE(retNode->valueExpr != nullptr);
        REQUIRE(retNode->valueExpr->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[retNode->valueExpr->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[retNode->valueExpr->tokenIndex].value.type() == TypedValue::Type::kSymbol);

        CHECK(classNode->next == nullptr);
    }

    SUBCASE("methoddef: '*' binop '{' argdecls funcvardecls primitive methbody '}'") {
        Parser parser("QRS { * !== { arg x = nil, y = true; var sd; var mm; _Pz ^nil; } }",
                std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("QRS") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());

        REQUIRE(classNode->methods != nullptr);
        const parse::MethodNode* method = classNode->methods.get();
        CHECK(method->methodName.compare("!==") == 0);
        CHECK(method->isClassMethod);
        CHECK(method->primitive.compare("_Pz") == 0);

        REQUIRE(method->arguments != nullptr);
        auto argList = method->arguments.get();
        REQUIRE(argList->varList != nullptr);
        REQUIRE(argList->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = argList->varList->definitions.get();
        CHECK(varDef->varName.compare("x") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        CHECK(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kNil);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("y") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        CHECK(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kBoolean);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asBoolean());
        CHECK(varDef->next == nullptr);

        REQUIRE(method->variables != nullptr);
        const parse::VarListNode* varList = method->variables.get();
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("sd") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
        REQUIRE(varList->next != nullptr);
        REQUIRE(varList->next->nodeType == parse::NodeType::kVarList);
        varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("mm") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
        CHECK(varList->next == nullptr);

        REQUIRE(method->body != nullptr);
        REQUIRE(method->body->nodeType == parse::NodeType::kReturn);
        auto retNode = reinterpret_cast<const parse::ReturnNode*>(method->body.get());
        REQUIRE(retNode->valueExpr != nullptr);
        REQUIRE(retNode->valueExpr->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[retNode->valueExpr->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[retNode->valueExpr->tokenIndex].value.type() == TypedValue::Type::kNil);

        CHECK(classNode->next == nullptr);
    }
}

TEST_CASE("Parser funcvardecls1") {
    SUBCASE("funcvardecls1: funcvardecl") {
        Parser parser("var x;", std::make_shared<ErrorReporter>());
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
        CHECK(varDef->varName.compare("x") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
        CHECK(block->variables->next == nullptr);
    }

    SUBCASE("funcvardecls1: funcvardecls1 funcvardecl") {
        Parser parser("var abc = 2; var d, e = 4, f;", std::make_shared<ErrorReporter>());
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
        CHECK(varDef->varName.compare("abc") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        CHECK(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asInteger() == 2);
        CHECK(varDef->next == nullptr);

        REQUIRE(block->variables->next != nullptr);
        REQUIRE(block->variables->next->nodeType == parse::NodeType::kVarList);
        auto varList = reinterpret_cast<const parse::VarListNode*>(block->variables->next.get());
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("d") == 0);
        CHECK(varDef->initialValue == nullptr);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("e") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        CHECK(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asInteger() == 4);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("f") == 0);
        CHECK(varDef->next == nullptr);

        CHECK(varList->next == nullptr);
    }
}

TEST_CASE("Parser funcvardecl") {
    SUBCASE("funcvardecl: VAR vardeflist ';'") {
        Parser parser("var first = \"abc\", second = \\zed, third;", std::make_shared<ErrorReporter>());
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
        CHECK(varDef->varName.compare("first") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        CHECK(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kString);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("second") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        CHECK(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kSymbol);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("third") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
    }
}

// funretval: <e> | '^' expr optsemi
TEST_CASE("Parser funcbody") {
    SUBCASE("funcbody: funretval") {
        Parser parser("^false", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kReturn);
        auto retNode = reinterpret_cast<const parse::ReturnNode*>(parser.root());
        REQUIRE(retNode->valueExpr != nullptr);
        REQUIRE(retNode->valueExpr->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[retNode->valueExpr->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[retNode->valueExpr->tokenIndex].value.type() == TypedValue::Type::kBoolean);
        CHECK(!parser.tokens()[retNode->valueExpr->tokenIndex].value.asBoolean());
        CHECK(retNode->next == nullptr);
    }

    SUBCASE("funcbody: exprseq funretval") {
        Parser parser("1; 'gar'; ^x", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kLiteral);
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(parser.root());
        CHECK(parser.tokens()[literal->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[literal->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[literal->tokenIndex].value.asInteger() == 1);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(literal->next.get());
        CHECK(parser.tokens()[literal->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[literal->tokenIndex].value.type() == TypedValue::Type::kSymbol);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kReturn);
        auto retNode = reinterpret_cast<const parse::ReturnNode*>(literal->next.get());
        REQUIRE(retNode->valueExpr != nullptr);
        REQUIRE(retNode->valueExpr->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(retNode->valueExpr.get());
        CHECK(name->name.compare("x") == 0);
        CHECK(name->next == nullptr);
    }
}

TEST_CASE("Parser rwslotdeflist") {
    SUBCASE("rwslotdeflist: rwslotdef") {
        Parser parser("M { var <> rw; }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("M") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("rw") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);
        CHECK(varDef->next == nullptr);

        CHECK(classNode->next == nullptr);
    }

    SUBCASE("rwslotdeflist ',' rwslotdef") {
        Parser parser("Cv { classvar a, < b, > c; }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("Cv") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kClassVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("a") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(!varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("b") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("c") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
        CHECK(!varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);
    }
}

// rwspec: <e> | '<' | READWRITEVAR | '>'
TEST_CASE("Parser rwslotdef") {
    SUBCASE("rwslotdef: rwspec name") {
        Parser parser("BFG { var prv_x; }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("BFG") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("prv_x") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(!varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
        CHECK(varDef->next == nullptr);

        CHECK(classNode->next == nullptr);
    }

    SUBCASE("rwslotdef: rwspec name '=' slotliteral") {
        Parser parser("Lit { var >ax = 2; }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("Lit") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kVar);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("ax") == 0);
        CHECK(!varDef->hasReadAccessor);
        CHECK(varDef->hasWriteAccessor);
        CHECK(varDef->next == nullptr);
        REQUIRE(varDef->initialValue != nullptr);
        CHECK(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asInteger() == 2);

        CHECK(classNode->next == nullptr);
    }
}

// optcomma: <e> | ','
TEST_CASE("Parser constdeflist") {
    SUBCASE("constdeflist: constdef") {
    }

    SUBCASE("constdeflist: constdeflist optcomma constdef") {
        Parser parser("MultiConst { const a = -1.0 <b = 2 < c = 3.0 }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("MultiConst") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kConst);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("a") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kFloat);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asFloat() == -1.0);
        CHECK(!varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("b") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kInteger);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asInteger() == 2);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("c") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].type == Lexer::Token::Type::kLiteral);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.type() == TypedValue::Type::kFloat);
        CHECK(parser.tokens()[varDef->initialValue->tokenIndex].value.asInteger() == 3.0);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
    }
}

// rspec: <e> | '<'
TEST_CASE("Parser constdef") {
    SUBCASE("constdef: rspec name '=' slotliteral") {
    }
}

TEST_CASE("Parser vardeflist") {
    SUBCASE("vardeflist: vardef") {
    }

    SUBCASE("vardeflist: vardeflist ',' vardef") {
    }
}

TEST_CASE("Parser vardef") {
    SUBCASE("vardef: name") {
    }

    SUBCASE("vardef: name '=' expr") {
    }

    SUBCASE("vardef: name '(' exprseq ')'") {
    }
}

TEST_CASE("Parser argdecls") {
    SUBCASE("argdecls: <e>") {
    }

    SUBCASE("argdecls: ARG vardeflist ';'") {
    }

    SUBCASE("argdecls: ARG vardeflist0 ELLIPSIS name ';'") {
    }

    SUBCASE("argdecls: '|' slotdeflist '|'") {
    }

    SUBCASE("argdecls: '|' slotdeflist0 ELLIPSIS name '|'") {
    }
}

// retval: <e> | '^' expr optsemi
TEST_CASE("Parser methbody") {
    SUBCASE("methbody: retval") {
    }

    SUBCASE("methbody: exprseq retval") {
    }
}

// exprn: expr | exprn ';' expr
TEST_CASE("Parser exprseq") {
    SUBCASE("exprseq: exprn optsemi") {
    }
}

TEST_CASE("Parser expr") {
    SUBCASE("expr: expr1") {
    }

    SUBCASE("expr: valrangexd") {
    }

    SUBCASE("expr: valrangeassign") {
    }

    SUBCASE("expr: classname") {
    }

    SUBCASE("expr: expr '.' '[' arglist1 ']'") {
    }

    SUBCASE("expr: '`' expr") {
    }

    SUBCASE("expr binop2 adverb expr") {
    }

    SUBCASE("expr: name '=' expr") {
    }

    SUBCASE("expr: '~' name '=' expr") {
    }

    SUBCASE("expr: expr '.' name '=' expr") {
    }

    SUBCASE("expr: name '(' arglist1 optkeyarglist ')' '=' expr") {
    }

    SUBCASE("expr: '#' mavars '=' expr") {
    }

    SUBCASE("expr: expr1 '[' arglist1 ']' '=' expr") {
    }

    SUBCASE("expr: expr '.' '[' arglist1 ']' '=' expr") {
    }
}

} // namespace hadron
