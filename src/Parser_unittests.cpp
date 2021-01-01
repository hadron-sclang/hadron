#include "Parser.hpp"

#include "ErrorReporter.hpp"

#include "doctest/doctest.h"
#include "spdlog/spdlog.h"

namespace hadron {

// root: classes | classextensions | cmdlinecode
TEST_CASE("Parser root") {
    SUBCASE("empty string") {
        Parser parser("", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        CHECK(parser.root()->nodeType == parse::NodeType::kEmpty);
        CHECK(parser.root()->tokenIndex == 0);
        CHECK(parser.root()->next.get() == nullptr);
        CHECK(parser.root()->tail == parser.root());
    }

    SUBCASE("root: classes") {
        Parser parser("A { }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->next == nullptr);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("A") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->variables == nullptr);
        CHECK(classNode->methods == nullptr);
    }

    SUBCASE("root: classextensions") {
        Parser parser("+ A { }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClassExt);
        CHECK(parser.root()->next == nullptr);
        CHECK(parser.root()->tail == parser.root());
        auto classExt = reinterpret_cast<const parse::ClassExtNode*>(parser.root());
        CHECK(classExt->className.compare("A") == 0);
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

} // namespace hadron
