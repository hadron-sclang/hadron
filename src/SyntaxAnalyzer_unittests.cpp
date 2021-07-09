#include "hadron/SyntaxAnalyzer.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Hash.hpp"

#include "doctest/doctest.h"

#include <memory>
#include <string_view>

namespace {
std::unique_ptr<hadron::SyntaxAnalyzer> buildFromSource(std::string_view code) {
    hadron::Parser parser(code);
    if (!parser.parse()) {
        return nullptr;
    }

    auto analyzer = std::make_unique<hadron::SyntaxAnalyzer>(parser.errorReporter());
    if (!analyzer->buildAST(&parser)) {
        return nullptr;
    }

    return analyzer;
}
} // namespace

namespace hadron {

TEST_CASE("Identifier Resolution") {
    SUBCASE("Local Variable") {
        auto analyzer = buildFromSource("( var a = 3; a )");
        REQUIRE(analyzer);
        REQUIRE(analyzer->ast()->astType == ast::ASTType::kBlock);
        const auto block = reinterpret_cast<const ast::BlockAST*>(analyzer->ast());
        CHECK(block->parent == nullptr);
        CHECK(block->arguments.size() == 0);
        CHECK(block->variables.size() == 1);
        auto value = block->variables.find(hash("a"));
        REQUIRE(value != block->variables.end());
        CHECK(value->second.name == "a");
        // Expecting two statements, the first to initialize the variable a to 3, the second to return the value of a.
        CHECK(block->statements.size() == 2);
        auto statement = block->statements.begin();
        REQUIRE((*statement)->astType == ast::ASTType::kAssign);
        const auto assign = reinterpret_cast<const ast::AssignAST*>(statement->get());
        REQUIRE(assign->target);
        CHECK(assign->target->nameHash == hash("a"));
        CHECK(assign->target->owningBlock == block);
        CHECK(assign->target->reference == value->second.references.begin());
        CHECK(assign->target->isWrite);
        REQUIRE(assign->value);
        REQUIRE(assign->value->astType == ast::ASTType::kConstant);
        const auto constant = reinterpret_cast<const ast::ConstantAST*>(assign->value.get());
        REQUIRE(constant->value.type() == Type::kInteger);
        CHECK(constant->value.asInteger() == 3);
        ++statement;
        REQUIRE((*statement)->astType == ast::ASTType::kResult);
        const auto result = reinterpret_cast<const ast::ResultAST*>(statement->get());
        REQUIRE(result->value);
        REQUIRE(result->value->astType == ast::ASTType::kValue);
        const auto retVal = reinterpret_cast<const ast::ValueAST*>(result->value.get());
        CHECK(retVal->nameHash == hash("a"));
        CHECK(retVal->owningBlock == block);
        CHECK(retVal->reference == (++value->second.references.begin()));
        CHECK(!retVal->isWrite);
    }

    SUBCASE("local variable in outer scope") {
    }

    SUBCASE("instance variable") {
    }

    SUBCASE("class variable") {
    }

    SUBCASE("class constant") {
    }

    SUBCASE("local variable hiding outer scope variable of same name") {
    }

    SUBCASE("local variable hiding instance variable of same name") {
    }

    SUBCASE("local variable in block vs argument to block") {
    }

    SUBCASE("local variable in block vs argument to outer block") {
    }
}

TEST_CASE("Type Deduction") {
    SUBCASE("var deduction from constant initializers") {
    }
    SUBCASE("var deduction from binary operations") {
    }
}

TEST_CASE("Binary Operator Lowering") {
    SUBCASE("Two Integer Arithmetic Operations") {
    }
}

TEST_CASE("Control Flow Lowering") {
}

TEST_CASE("Implied Return Assignment") {
}

TEST_CASE("Inlining Literal Blocks") {
    SUBCASE("variable scope hoisting") {
    }
}

TEST_CASE("Class Generation") {
    SUBCASE("read accessor for instance variable") {
    }

    SUBCASE("write accessor for instance variable") {
    }

    SUBCASE("read accessor for class variable") {
    }

    SUBCASE("write accessor for class variable") {
    }

    SUBCASE("read accessor for constant") {
    }
}

} // namespace hadron