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
        CHECK(literal->tokenIndex == 0);
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 42);
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
        Parser parser("Sub [ opt ] : Super { const c = -5; *meth { } }", std::make_shared<ErrorReporter>());
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
        auto literal = reinterpret_cast<const parse::LiteralNode*>(
                classNode->variables->definitions->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == -5);
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
        auto literal = reinterpret_cast<const parse::LiteralNode*>(block->body.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 0xa);
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
        auto literal = reinterpret_cast<const parse::LiteralNode*>(block->variables->definitions->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kSymbol);
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
        auto literal = reinterpret_cast<const parse::LiteralNode*>(parser.root());
        CHECK(literal->value.type() == TypedLiteral::Type::kString);
        CHECK(parser.root()->next == nullptr);
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
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 1);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("h") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 2);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("i") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 3);

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
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 42);
        CHECK(!varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("red5") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kString);
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
        REQUIRE(method->body->nodeType == parse::NodeType::kLiteral);
        auto literal = reinterpret_cast<const parse::LiteralNode*>(method->body.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 17);

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
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 5);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("n") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 7);
        CHECK(varDef->next == nullptr);

        REQUIRE(method->variables != nullptr);
        const parse::VarListNode* varList = method->variables.get();
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("k") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 0);
        CHECK(varDef->next == nullptr);
        REQUIRE(varList->next != nullptr);
        REQUIRE(varList->next->nodeType == parse::NodeType::kVarList);
        varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        REQUIRE(varList->definitions != nullptr);
        varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("z") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kSymbol);
        CHECK(varDef->next == nullptr);
        CHECK(varList->next == nullptr);

        REQUIRE(method->body != nullptr);
        REQUIRE(method->body->nodeType == parse::NodeType::kReturn);
        auto retNode = reinterpret_cast<const parse::ReturnNode*>(method->body.get());
        REQUIRE(retNode->valueExpr != nullptr);
        REQUIRE(retNode->valueExpr->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(retNode->valueExpr.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kSymbol);

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
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kNil);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("y") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kBoolean);
        CHECK(literal->value.asBoolean());
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
        literal = reinterpret_cast<const parse::LiteralNode*>(retNode->valueExpr.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kNil);

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
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 2);
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
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 4);
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
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kString);
        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("second") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kSymbol);
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
        auto literal = reinterpret_cast<const parse::LiteralNode*>(retNode->valueExpr.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kBoolean);
        CHECK(!literal->value.asBoolean());
        CHECK(retNode->next == nullptr);
    }

    SUBCASE("funcbody: exprseq funretval") {
        Parser parser("1; 'gar'; ^x", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kExprSeq);
        auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(parser.root());

        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kLiteral);
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(exprSeq->expr.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 1);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(literal->next.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kSymbol);

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
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        auto literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 2);

        CHECK(classNode->next == nullptr);
    }
}

// optcomma: <e> | ','
TEST_CASE("Parser constdeflist") {
    SUBCASE("constdeflist: constdef") {
        Parser parser("UniConst { const psi=\"psi\"; }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("UniConst") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kConst);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("psi") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        auto literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kString);
        CHECK(!varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
    }

    SUBCASE("constdeflist: constdeflist optcomma constdef") {
        Parser parser("MultiConst { const a = -1.0 <b=2 < c = 3.0; }", std::make_shared<ErrorReporter>());
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
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kFloat);
        CHECK(literal->value.asFloat() == -1.0);
        CHECK(!varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("b") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 2);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("c") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kFloat);
        CHECK(literal->value.asFloat() == 3.0);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
    }
}

// rspec: <e> | '<'
TEST_CASE("Parser constdef") {
    SUBCASE("constdef: rspec name '=' slotliteral") {
        Parser parser("Math { const <epsilon= -0.0001; }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kClass);
        CHECK(parser.root()->tail == parser.root());
        auto classNode = reinterpret_cast<const parse::ClassNode*>(parser.root());
        CHECK(classNode->className.compare("Math") == 0);
        CHECK(classNode->superClassName.empty());
        CHECK(classNode->optionalName.empty());
        CHECK(classNode->methods == nullptr);

        REQUIRE(classNode->variables != nullptr);
        const parse::VarListNode* varList = classNode->variables.get();
        CHECK(parser.tokens()[varList->tokenIndex].type == Lexer::Token::Type::kConst);

        REQUIRE(varList->definitions != nullptr);
        const parse::VarDefNode* varDef = varList->definitions.get();
        CHECK(varDef->varName.compare("epsilon") == 0);
        REQUIRE(varDef->initialValue != nullptr);
        REQUIRE(varDef->initialValue->nodeType == parse::NodeType::kLiteral);
        auto literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kFloat);
        CHECK(literal->value.asFloat() == -0.0001);
        CHECK(varDef->hasReadAccessor);
        CHECK(!varDef->hasWriteAccessor);
    }
}

TEST_CASE("Parser vardeflist") {
    SUBCASE("vardeflist: vardef") {
        Parser parser("( var ax7; )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        CHECK(block->variables->definitions->varName.compare("ax7") == 0);
        CHECK(block->variables->definitions->initialValue == nullptr);
        CHECK(block->variables->definitions->next == nullptr);
        CHECK(block->variables->next == nullptr);
        CHECK(block->body == nullptr);
    }

    SUBCASE("vardeflist: vardeflist ',' vardef") {
        Parser parser("( var m,n,o,p; )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        const parse::VarDefNode* varDef = block->variables->definitions.get();
        CHECK(varDef->varName.compare("m") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("n") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("o") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("p") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
    }
}

TEST_CASE("Parser vardef") {
    SUBCASE("vardef: name") {
        Parser parser("( var very_long_name_with_numbers_12345; )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        CHECK(block->variables->definitions->varName.compare("very_long_name_with_numbers_12345") == 0);
        CHECK(block->variables->definitions->initialValue == nullptr);
        CHECK(block->variables->definitions->next == nullptr);
        CHECK(block->variables->next == nullptr);
        CHECK(block->body == nullptr);
    }

    SUBCASE("vardef: name '=' expr") {
        Parser parser("( var x = -5.8; )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        CHECK(block->variables->definitions->varName.compare("x") == 0);
        REQUIRE(block->variables->definitions->initialValue != nullptr);
        REQUIRE(block->variables->definitions->initialValue->nodeType == parse::NodeType::kLiteral);
        auto literal = reinterpret_cast<const parse::LiteralNode*>(block->variables->definitions->initialValue.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kFloat);
        CHECK(literal->value.asFloat() == -5.8);

        CHECK(block->variables->definitions->next == nullptr);
        CHECK(block->variables->next == nullptr);
        CHECK(block->body == nullptr);
    }

    SUBCASE("vardef: name '(' exprseq ')'") {
        Parser parser("( var seq(1; 2); )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        CHECK(parser.root()->next == nullptr);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);

        REQUIRE(block->variables != nullptr);
        REQUIRE(block->variables->definitions);
        CHECK(block->variables->definitions->varName.compare("seq") == 0);
        REQUIRE(block->variables->definitions->initialValue != nullptr);
        REQUIRE(block->variables->definitions->initialValue->nodeType == parse::NodeType::kExprSeq);
        auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(block->variables->definitions->initialValue.get());
        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kLiteral);
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(exprSeq->expr.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 1);
        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(literal->next.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 2);

        CHECK(exprSeq->next == nullptr);
        CHECK(block->variables->next == nullptr);
        CHECK(block->body == nullptr);
    }
}

TEST_CASE("Parser argdecls") {
    SUBCASE("argdecls: <e>") {
        Parser parser("{ 1 }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kLiteral);
        auto literal = reinterpret_cast<const parse::LiteralNode*>(block->body.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 1);
        CHECK(literal->next == nullptr);
    }

    SUBCASE("argdecls: ARG vardeflist ';'") {
        Parser parser("{ arg arg1, arg2, arg3; }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);
        CHECK(block->body == nullptr);

        REQUIRE(block->arguments != nullptr);
        CHECK(block->arguments->varArgsName.empty());
        REQUIRE(block->arguments->varList != nullptr);

        REQUIRE(block->arguments->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = block->arguments->varList->definitions.get();
        CHECK(varDef->varName.compare("arg1") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("arg2") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("arg3") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
    }

    SUBCASE("argdecls: ARG vardeflist0 ELLIPSIS name ';'") {
        Parser parser("{ arg x, y, z ... w; }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);
        CHECK(block->body == nullptr);

        REQUIRE(block->arguments != nullptr);
        CHECK(block->arguments->varArgsName.compare("w") == 0);
        REQUIRE(block->arguments->varList != nullptr);

        REQUIRE(block->arguments->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = block->arguments->varList->definitions.get();
        CHECK(varDef->varName.compare("x") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("y") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("z") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
    }

    SUBCASE("argdecls: '|' slotdeflist '|'") {
        Parser parser("{ |i,j,k| }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);
        CHECK(block->body == nullptr);

        REQUIRE(block->arguments != nullptr);
        CHECK(block->arguments->varArgsName.empty());
        REQUIRE(block->arguments->varList != nullptr);

        REQUIRE(block->arguments->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = block->arguments->varList->definitions.get();
        CHECK(varDef->varName.compare("i") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("j") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("k") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
    }

    SUBCASE("argdecls: '|' slotdeflist0 ELLIPSIS name '|'") {
        Parser parser("{ |i0,j1,k2...w3| }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);
        CHECK(block->body == nullptr);

        REQUIRE(block->arguments != nullptr);
        CHECK(block->arguments->varArgsName.compare("w3") == 0);
        REQUIRE(block->arguments->varList != nullptr);

        REQUIRE(block->arguments->varList->definitions != nullptr);
        const parse::VarDefNode* varDef = block->arguments->varList->definitions.get();
        CHECK(varDef->varName.compare("i0") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("j1") == 0);
        CHECK(varDef->initialValue == nullptr);

        REQUIRE(varDef->next != nullptr);
        REQUIRE(varDef->next->nodeType == parse::NodeType::kVarDef);
        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        CHECK(varDef->varName.compare("k2") == 0);
        CHECK(varDef->initialValue == nullptr);
        CHECK(varDef->next == nullptr);
    }
}

// retval: <e> | '^' expr optsemi
TEST_CASE("Parser methbody") {
    SUBCASE("methbody: retval") {
        Parser parser("{ ^this }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kReturn);
        auto retVal = reinterpret_cast<const parse::ReturnNode*>(block->body.get());
        REQUIRE(retVal->valueExpr != nullptr);
        REQUIRE(retVal->valueExpr->nodeType == parse::NodeType::kName);
        auto nameNode = reinterpret_cast<const parse::NameNode*>(retVal->valueExpr.get());
        CHECK(nameNode->name.compare("this") == 0);
    }

    SUBCASE("methbody: exprseq retval") {
        Parser parser("{ 1; 2; ^3; }", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kExprSeq);
        auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(block->body.get());

        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kLiteral);
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(exprSeq->expr.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 1);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(literal->next.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 2);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kReturn);
        auto retVal = reinterpret_cast<const parse::ReturnNode*>(literal->next.get());
        REQUIRE(retVal->valueExpr != nullptr);
        REQUIRE(retVal->valueExpr->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(retVal->valueExpr.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 3);
    }
}

// exprn: expr | exprn ';' expr
TEST_CASE("Parser exprseq") {
    SUBCASE("exprseq: exprn optsemi") {
        Parser parser("( x; y; z )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kExprSeq);
        auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(block->body.get());

        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kName);
        const parse::NameNode* nameNode = reinterpret_cast<const parse::NameNode*>(exprSeq->expr.get());
        CHECK(nameNode->name.compare("x") == 0);

        REQUIRE(nameNode->next != nullptr);
        REQUIRE(nameNode->next->nodeType == parse::NodeType::kName);
        nameNode = reinterpret_cast<const parse::NameNode*>(nameNode->next.get());
        CHECK(nameNode->name.compare("y") == 0);

        REQUIRE(nameNode->next != nullptr);
        REQUIRE(nameNode->next->nodeType == parse::NodeType::kName);
        nameNode = reinterpret_cast<const parse::NameNode*>(nameNode->next.get());
        CHECK(nameNode->name.compare("z") == 0);
        CHECK(nameNode->next == nullptr);
    }
}

TEST_CASE("Parser msgsend") {
    SUBCASE("msgsend: name blocklist1") {
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
    }

    SUBCASE("msgsend: classname '(' ')' blocklist") {
    }

    SUBCASE("msgsend: classname '(' keyarglist1 optcomma ')' blocklist") {
    }

    SUBCASE("msgsend: classname '(' arglist1 optkeyarglist ')' blocklist") {
    }

    SUBCASE("msgsend: classname '(' arglistv1 optkeyarglist ')'") {
    }

    SUBCASE("msgsend: expr '.' '(' ')' blocklist") {
    }

    SUBCASE("msgsend: expr '.' '(' keyarglist1 optcomma ')' blocklist") {
    }

    SUBCASE("msgsend: expr '.' name '(' keyarglist1 optcomma ')' blocklist") {
    }

    SUBCASE("msgsend: expr '.' '(' arglist1 optkeyarglist ')' blocklist") {
    }

    SUBCASE("msgsend: expr '.' '(' arglistv1 optkeyarglist ')'") {
    }

    SUBCASE("msgsend: expr '.' name '(' ')' blocklist") {
    }

    SUBCASE("msgsend: expr '.' name '(' arglist1 optkeyarglist ')' blocklist") {
    }

    SUBCASE("msgsend: expr '.' name '(' arglistv1 optkeyarglist ')'") {
    }

    SUBCASE("msgsend: expr '.' name blocklist") {
    }
}

TEST_CASE("Parser expr") {
    SUBCASE("expr: expr1") {
        Parser parser("( \\g )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kLiteral);
        auto literal = reinterpret_cast<const parse::LiteralNode*>(block->body.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kSymbol);
    }

    SUBCASE("expr: valrangexd") {
    }

    SUBCASE("expr: valrangeassign") {
    }

    SUBCASE("expr: classname") {
        Parser parser("( Object )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(block->body.get());
        CHECK(name->name.compare("Object") == 0);
        CHECK(!name->isGlobal);
    }

    SUBCASE("expr: expr '.' '[' arglist1 ']'") {
    }

    SUBCASE("expr: '`' expr") {
    }

    SUBCASE("expr binop2 adverb expr") {
        Parser parser("( a + b not: c )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kBinopCall);
        const parse::BinopCallNode* binop = reinterpret_cast<const parse::BinopCallNode*>(block->body.get());
        CHECK(binop->selector.compare("+") == 0);
        REQUIRE(binop->leftHand != nullptr);
        REQUIRE(binop->leftHand->nodeType == parse::NodeType::kName);
        const parse::NameNode* name = reinterpret_cast<const parse::NameNode*>(binop->leftHand.get());
        CHECK(name->name.compare("a") == 0);

        REQUIRE(binop->rightHand != nullptr);
        REQUIRE(binop->rightHand->nodeType == parse::NodeType::kBinopCall);
        binop = reinterpret_cast<const parse::BinopCallNode*>(binop->rightHand.get());
        CHECK(binop->selector.compare("not") == 0);
        REQUIRE(binop->leftHand != nullptr);
        REQUIRE(binop->leftHand->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(binop->leftHand.get());
        CHECK(name->name.compare("b") == 0);
        REQUIRE(binop->rightHand != nullptr);
        REQUIRE(binop->rightHand->nodeType == parse::NodeType::kName);
        name = reinterpret_cast<const parse::NameNode*>(binop->rightHand.get());
        CHECK(name->name.compare("c") == 0);
    }

    SUBCASE("expr: name '=' expr") {
        Parser parser("( four = 4 )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kAssign);
        auto assign = reinterpret_cast<const parse::AssignNode*>(block->body.get());
        CHECK(assign->name->name.compare("four") == 0);
        CHECK(!assign->name->isGlobal);
        CHECK(assign->name->next == nullptr);
        REQUIRE(assign->value != nullptr);
        REQUIRE(assign->value->nodeType == parse::NodeType::kLiteral);
        auto literal = reinterpret_cast<const parse::LiteralNode*>(assign->value.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 4);
    }

    SUBCASE("expr: '~' name '=' expr") {
        Parser parser("( ~globez = \"xyz\" )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kAssign);
        auto assign = reinterpret_cast<const parse::AssignNode*>(block->body.get());
        CHECK(assign->name->name.compare("globez") == 0);
        CHECK(assign->name->isGlobal);
        CHECK(assign->name->next == nullptr);
        REQUIRE(assign->value != nullptr);
        REQUIRE(assign->value->nodeType == parse::NodeType::kLiteral);
        auto literal = reinterpret_cast<const parse::LiteralNode*>(assign->value.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kString);
    }

    SUBCASE("expr: expr '.' name '=' expr") {
        Parser parser("( ~object.property = true )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kSetter);
        auto setter = reinterpret_cast<const parse::SetterNode*>(block->body.get());
        CHECK(setter->selector.compare("property") == 0);
        CHECK(setter->next == nullptr);

        REQUIRE(setter->target != nullptr);
        REQUIRE(setter->target->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(setter->target.get());
        CHECK(name->isGlobal);
        CHECK(name->name.compare("object") == 0);

        REQUIRE(setter->value != nullptr);
        REQUIRE(setter->value->nodeType == parse::NodeType::kLiteral);
        auto literal = reinterpret_cast<const parse::LiteralNode*>(setter->value.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kBoolean);
        CHECK(literal->value.asBoolean());
    }

    SUBCASE("expr: name '(' arglist1 optkeyarglist ')' '=' expr") {
    }

    SUBCASE("expr: '#' mavars '=' expr") {
        // #a, b, c = [1, 2, 3];

    }

    SUBCASE("expr: expr1 '[' arglist1 ']' '=' expr") {
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
    }

    SUBCASE("expr1: msgsend") {
    }

    SUBCASE("expr1: '(' exprseq ')'") {
    }

    SUBCASE("expr1: '~' name") {
        Parser parser("( ~z )", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());
        CHECK(block->arguments == nullptr);
        CHECK(block->variables == nullptr);
        CHECK(block->next == nullptr);

        REQUIRE(block->body != nullptr);
        REQUIRE(block->body->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(block->body.get());
        CHECK(name->isGlobal);
        CHECK(name->name.compare("z") == 0);
    }

    SUBCASE("expr1: '[' arrayelems ']'") {
    }

    SUBCASE("expr1: '(' valrange2 ')'") {
    }

    SUBCASE("expr1: '(' ':' valrange3 ')'") {
    }

    SUBCASE("expr1: '(' dictslotlist ')'") {
    }

    SUBCASE("expr1: pseudovar") {
    }

    SUBCASE("expr1: expr1 '[' arglist1 ']'") {
    }

    SUBCASE("expr1: valrangexd") {
    }
}

TEST_CASE("Parser literal") {
    SUBCASE("literal: '-' INTEGER") {
        Parser parser("- /*****/ 1", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kLiteral);
        auto literal = reinterpret_cast<const parse::LiteralNode*>(parser.root());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == -1);
    }
}

TEST_CASE("Parser arrayelems") {
    SUBCASE("arrayelems: <e>") {
        Parser parser("[ ]", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kDynList);
        auto array = reinterpret_cast<const parse::DynListNode*>(parser.root());
        CHECK(array->elements == nullptr);
    }

    SUBCASE("arrayelems: arrayelems1 optcomma") {
        Parser parser("[1,-2,]", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kDynList);
        auto array = reinterpret_cast<const parse::DynListNode*>(parser.root());

        REQUIRE(array->elements != nullptr);
        REQUIRE(array->elements->nodeType == parse::NodeType::kLiteral);
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(array->elements.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 1);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(literal->next.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == -2);
    }
}

TEST_CASE("Parser arrayelems1") {
    SUBCASE("arrayelems1: exprseq") {
        Parser parser("[ 3; a; nil, ]", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kDynList);
        auto array = reinterpret_cast<const parse::DynListNode*>(parser.root());

        REQUIRE(array->elements != nullptr);
        REQUIRE(array->elements->nodeType == parse::NodeType::kExprSeq);
        auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(array->elements.get());

        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kLiteral);
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(exprSeq->expr.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 3);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kName);
        auto name = reinterpret_cast<const parse::NameNode*>(literal->next.get());
        CHECK(name->name.compare("a") == 0);

        REQUIRE(name->next != nullptr);
        REQUIRE(name->next->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(name->next.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kNil);
        CHECK(literal->next == nullptr);
    }

    SUBCASE("arrayelems1: exprseq ':' exprseq") {
        Parser parser("[ 1;2: 3;4 ]", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kDynList);
        auto array = reinterpret_cast<const parse::DynListNode*>(parser.root());

        REQUIRE(array->elements != nullptr);
        REQUIRE(array->elements->nodeType == parse::NodeType::kExprSeq);
        const parse::ExprSeqNode* exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(array->elements.get());

        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kLiteral);
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(exprSeq->expr.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 1);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(literal->next.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 2);

        REQUIRE(exprSeq->next != nullptr);
        REQUIRE(exprSeq->next->nodeType == parse::NodeType::kExprSeq);
        exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(exprSeq->next.get());

        REQUIRE(exprSeq->expr != nullptr);
        REQUIRE(exprSeq->expr->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(exprSeq->expr.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 3);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(literal->next.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 4);
    }

    SUBCASE("keybinop exprseq") {
        Parser parser("[freq: 440,]", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());

        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kDynList);
        auto array = reinterpret_cast<const parse::DynListNode*>(parser.root());

        REQUIRE(array->elements != nullptr);
        REQUIRE(array->elements->nodeType == parse::NodeType::kLiteral);
        const parse::LiteralNode* literal = reinterpret_cast<const parse::LiteralNode*>(array->elements.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kSymbol);

        REQUIRE(literal->next != nullptr);
        REQUIRE(literal->next->nodeType == parse::NodeType::kLiteral);
        literal = reinterpret_cast<const parse::LiteralNode*>(literal->next.get());
        CHECK(literal->value.type() == TypedLiteral::Type::kInteger);
        CHECK(literal->value.asInteger() == 440);
        CHECK(literal->next == nullptr);
    }

    SUBCASE("arrayelems1 ',' exprseq") {
    }

    SUBCASE("arrayelems1 ',' keybinop exprseq") {
    }

    SUBCASE("arrayelems1 ',' exprseq ':' exprseq") {
    }
}


} // namespace hadron
