#include "Lexer.hpp"

#include "doctest/doctest.h"

#include <vector>

namespace hadron {

TEST_CASE("Lexer Base Cases") {
    SUBCASE("empty string") {
        Lexer lexer("");
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 0);
    }
    SUBCASE("whitespace only") {
        Lexer lexer("   \t\n\r  ");
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 0);
    }
}

TEST_CASE("Lexer Integers Simple") {
    SUBCASE("zero") {
        const char* code = "0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.integer == 0);
    }
    SUBCASE("zero-padded zero") {
        const char* code = "000";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.integer == 0);
    }
    SUBCASE("whitespace-padded zero") {
        const char* code = "\n\t 0\r\t";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(*(lexer.tokens()[0].start) == '0');
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.integer == 0);
    }
    SUBCASE("single digit") {
        const char* code = "4";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.integer == 4);
    }
    SUBCASE("zero-padded single digit") {
        const char* code = "007";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.integer == 7);
    }
    SUBCASE("whitespace-padded single digit") {
        const char* code = "     9\t";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(*(lexer.tokens()[0].start) == '9');
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.integer == 9);
    }
    SUBCASE("multi digit") {
        const char* code = "991157";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 6);
        CHECK(lexer.tokens()[0].value.integer == 991157);
    }
    SUBCASE("zero padded") {
        const char* code = "0000000000000000043";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 19);
        CHECK(lexer.tokens()[0].value.integer == 43);
    }
    SUBCASE("whitespace padded") {
        const char* code = "    869  ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code + 4);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.integer == 869);
    }
    SUBCASE("bigger at 32 bits") {
        const char* code = "2147483647";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 10);
        CHECK(lexer.tokens()[0].value.integer == 2147483647);
    }
    SUBCASE("bigger than 32 bits <DIFFA0>") {
        const char* code = "2147483648";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 10);
        CHECK(lexer.tokens()[0].value.integer == 2147483648);
    }
}

TEST_CASE("Lexer Hexadecimal Integers") {
    SUBCASE("zero") {
        const char* code = "0x0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.integer == 0);
    }
    SUBCASE("single digit alpha") {
        const char* code = "0xa";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.integer == 10);
    }
    SUBCASE("single digit numeric") {
        const char* code = "0x2";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.integer == 2);
    }
    SUBCASE("multi-digit upper") {
        const char* code = "0xAAE724F";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 9);
        CHECK(lexer.tokens()[0].value.integer == 0xAAE724F);
    }
    SUBCASE("multi-digit lower") {
        const char* code = "0x42deadbeef42";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 14);
        CHECK(lexer.tokens()[0].value.integer == 0x42deadbeef42);
    }
    SUBCASE("zero padding") {
        const char* code = "000x742a";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 8);
        CHECK(lexer.tokens()[0].value.integer == 0x742a);
    }
    SUBCASE("whitespace padding") {
        const char* code = "    0x1234   ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code + 4);
        CHECK(lexer.tokens()[0].length == 6);
        CHECK(lexer.tokens()[0].value.integer == 0x1234);
    }
    SUBCASE("large value <DIFFA0>") {
        const char* code = "0x42deadbeef42";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 14);
        CHECK(lexer.tokens()[0].value.integer == 0x42deadbeef42);
    }
}

} // namespace hadron
