#include "Lexer.hpp"

#include "doctest/doctest.h"

#include <cstring>
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

TEST_CASE("Lexer Integers") {
    SUBCASE("zero") {
        const char* code = "0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
    }
    SUBCASE("zero-padded zero") {
        const char* code = "000";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
    }
    SUBCASE("whitespace-padded zero") {
        const char* code = "\n\t 0\r\t";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(*(lexer.tokens()[0].start) == '0');
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
    }
    SUBCASE("single digit") {
        const char* code = "4";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 4);
    }
    SUBCASE("zero-padded single digit") {
        const char* code = "007";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 7);
    }
    SUBCASE("whitespace-padded single digit") {
        const char* code = "     9\t";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(*(lexer.tokens()[0].start) == '9');
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 9);
    }
    SUBCASE("multi digit") {
        const char* code = "991157";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 6);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 991157);
    }
    SUBCASE("zero padded") {
        const char* code = "0000000000000000043";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 19);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 43);
    }
    SUBCASE("whitespace padded") {
        const char* code = "    869  ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 4);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 869);
    }
    SUBCASE("near 32 bit limit") {
        const char* code = "2147483647";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 10);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 2147483647);
    }
    SUBCASE("above 32 bits <DIFFA0>") {
        const char* code = "2147483648";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 10);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 2147483648);
    }
    SUBCASE("int list") {
        const char* code = "1,2, 3, 4";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 7);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[1].start == code + 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 2);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 2);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[3].start == code + 3);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[4].start == code + 5);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[4].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[4].value.asInteger() == 3);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[5].start == code + 6);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[6].start == code + 8);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[6].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[6].value.asInteger() == 4);
    }
    SUBCASE("int method call") {
        const char* code = "10.asString;";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 4);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 2);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 10);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kDot);
        CHECK(lexer.tokens()[1].start == code + 2);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[2].start == code + 3);
        CHECK(lexer.tokens()[2].length == 8);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kSemicolon);
        CHECK(lexer.tokens()[3].start == code + 11);
        CHECK(lexer.tokens()[3].length == 1);
    }
}

TEST_CASE("Lexer Floating Point") {
    SUBCASE("float zero") {
        const char* code = "0.0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kFloat);
        CHECK(lexer.tokens()[0].value.asFloat() == 0);
    }
    SUBCASE("leading zeros") {
        const char* code = "000.25";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 6);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kFloat);
        CHECK(lexer.tokens()[0].value.asFloat() == 0.25);
    }
    SUBCASE("integer and fraction") {
        const char* code = "987.125";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 7);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kFloat);
        CHECK(lexer.tokens()[0].value.asFloat() == 987.125);
    }
    SUBCASE("float method call") {
        const char* code = "1.23.asString";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 4);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kFloat);
        CHECK(lexer.tokens()[0].value.asFloat() == 1.23);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kDot);
        CHECK(lexer.tokens()[1].start == code + 4);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[2].start == code + 5);
        CHECK(lexer.tokens()[2].length == 8);
    }
}

TEST_CASE("Lexer Hexadecimal Integers") {
    SUBCASE("zero") {
        const char* code = "0x0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
    }
    SUBCASE("zero elided <DIFFA1>") {
        const char* code = "0x";
        Lexer lexer(code);
        // Will lex as two tokens, an integer 0 and an identifier 'x'
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[1].start == code + 1);
        CHECK(lexer.tokens()[1].length == 1);
    }
    SUBCASE("single digit alpha") {
        const char* code = "0xa";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 10);
    }
    SUBCASE("single digit numeric") {
        const char* code = "0x2";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 2);
    }
    SUBCASE("multi-digit upper") {
        const char* code = "0xAAE724F";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 9);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0xAAE724F);
    }
    SUBCASE("multi-digit lower") {
        const char* code = "0x42deadbeef42";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 14);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0x42deadbeef42);
    }
    SUBCASE("multi-digit mixed") {
        const char* code = "0x1A2b3C";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 8);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0x1a2b3c);
    }
    SUBCASE("zero padding <DIFFA2>") {
        const char* code = "000x742a";
        Lexer lexer(code);
        // Lexer will lex "000" as an integer and "x742a" as an identifier.
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[1].start == code + 3);
        CHECK(lexer.tokens()[1].length == 5);
    }
    SUBCASE("nonzero padding <DIFFA2>") {
        const char* code = "12345x1";
        Lexer lexer(code);
        // Lexer will lex "12345" as an integer and "x1" as an identifier.
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 5);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 12345);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[1].start == code + 5);
        CHECK(lexer.tokens()[1].length == 2);
    }
    SUBCASE("whitespace padding") {
        const char* code = "    0x1234   ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 4);
        CHECK(lexer.tokens()[0].length == 6);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0x1234);
    }
    SUBCASE("large value <DIFFA0>") {
        const char* code = "0x42deadbeef42";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 14);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0x42deadbeef42);
    }
}

TEST_CASE("Lexer Strings") {
    SUBCASE("empty string") {
        const char* code = "\"\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 0);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kString);
    }
    SUBCASE("simple string") {
        const char* code = "\"abc\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kString);
    }
    SUBCASE("padded string") {
        const char* code = "  \"Spaces inside and out.\"  ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 3);
        CHECK(lexer.tokens()[0].length == 22);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kString);
    }
    SUBCASE("escape characters") {
        const char* code = "\"\t\n\r\\t\\r\\n\\\"0x'\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 10);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kString);
    }
    SUBCASE("adjacent strings tight") {
        const char* code = "\"a\"\"b\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kString);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[1].start == code + 4);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[1].value.type() == TypedLiteral::Type::kString);
    }
    SUBCASE("adjacent strings padded") {
        const char* code = "  \"\\\"\"  \"b\"  ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 3);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kString);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[1].start == code + 9);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[1].value.type() == TypedLiteral::Type::kString);
    }
    SUBCASE("extended characters in string") {
        const char* code = "\"(╯°□°)╯︵ ┻━┻\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == std::strlen(code) - 2);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kString);
    }
    SUBCASE("unterminated string") {
        Lexer lexer("\"abc");
        REQUIRE(!lexer.lex());
    }
}

TEST_CASE("Lexer Symbols") {
    SUBCASE("empty quote symbol") {
        const char* code = "''";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 0);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kSymbol);
    }
    SUBCASE("simple quote") {
        const char* code = "'bA1'";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kSymbol);
    }
    SUBCASE("padded quote") {
        const char* code = "  'ALL CAPS READS LIKE SHOUTING'  ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 3);
        CHECK(lexer.tokens()[0].length == 28);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kSymbol);
    }
    SUBCASE("special characters") {
        const char* code = "'\\t\\n\\r\t\n\r\\'0x\"'";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 10);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kSymbol);
    }
    SUBCASE("unterminated quote") {
        Lexer lexer("'abc");
        REQUIRE(!lexer.lex());
    }
    SUBCASE("empty slash") {
        const char* code = "\\";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 0);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kSymbol);
    }
    SUBCASE("empty slash with whitespace") {
        const char* code = "\\ ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 0);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kSymbol);
    }
    SUBCASE("simple slash") {
        const char* code = "\\abcx_1234_ABCX";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 14);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kSymbol);
    }
    SUBCASE("symbol sequence") {
        const char* code = "'A', \\b , 'c',\\D,'e'";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 9);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kSymbol);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[1].start == code + 3);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 6);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kSymbol);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[3].start == code + 8);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[4].start == code + 11);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[4].value.type() == TypedLiteral::Type::kSymbol);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[5].start == code + 13);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[6].start == code + 15);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[6].value.type() == TypedLiteral::Type::kSymbol);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[7].length == 1);
        CHECK(lexer.tokens()[7].start == code + 16);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[8].start == code + 18);
        CHECK(lexer.tokens()[8].length == 1);
        CHECK(lexer.tokens()[8].value.type() == TypedLiteral::Type::kSymbol);
    }
    SUBCASE("extended characters in quote symbols") {
        const char* code = "'🖤💛💙💜💚🧡'";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == std::strlen(code) - 2);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kSymbol);
    }
}

TEST_CASE("Binary Operators") {
    SUBCASE("bare plus") {
        const char* code = "+ - * = < > | <> <-";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 9);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kPlus);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kMinus);
        CHECK(lexer.tokens()[1].start == code + 2);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kAsterisk);
        CHECK(lexer.tokens()[2].start == code + 4);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].couldBeBinop);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kAssign);
        CHECK(lexer.tokens()[3].start == code + 6);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[3].couldBeBinop);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kLessThan);
        CHECK(lexer.tokens()[4].start == code + 8);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[4].couldBeBinop);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kGreaterThan);
        CHECK(lexer.tokens()[5].start == code + 10);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[5].couldBeBinop);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kPipe);
        CHECK(lexer.tokens()[6].start == code + 12);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[6].couldBeBinop);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kReadWriteVar);
        CHECK(lexer.tokens()[7].start == code + 14);
        CHECK(lexer.tokens()[7].length == 2);
        CHECK(lexer.tokens()[7].couldBeBinop);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kLeftArrow);
        CHECK(lexer.tokens()[8].start == code + 17);
        CHECK(lexer.tokens()[8].length == 2);
        CHECK(lexer.tokens()[8].couldBeBinop);
    }
    SUBCASE("two integers padded") {
        const char* code = "1 + -22";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 4);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 1);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kPlus);
        CHECK(lexer.tokens()[1].start == code + 2);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kMinus);
        CHECK(lexer.tokens()[2].start == code + 4);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].couldBeBinop);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[3].start == code + 5);
        CHECK(lexer.tokens()[3].length == 2);
        CHECK(lexer.tokens()[3].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[3].value.asInteger() == 22);
        CHECK(!lexer.tokens()[3].couldBeBinop);
    }
    SUBCASE("two integers tight") {
        const char* code = "67!=4";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 2);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 67);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kBinop);
        CHECK(lexer.tokens()[1].start == code + 2);
        CHECK(lexer.tokens()[1].length == 2);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 4);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 4);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("tight left") {
        const char* code = "7+/+ 0x17";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 7);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kBinop);
        CHECK(lexer.tokens()[1].start == code + 1);
        CHECK(lexer.tokens()[1].length == 3);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 5);
        CHECK(lexer.tokens()[2].length == 4);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 0x17);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("tight right") {
        const char* code = "0xffe *93";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 5);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0xffe);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kAsterisk);
        CHECK(*(lexer.tokens()[1].start) == '*');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 7);
        CHECK(lexer.tokens()[2].length == 2);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 93);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("zeros tight") {
        const char* code = "0<-0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kLeftArrow);
        CHECK(lexer.tokens()[1].start == code + 1);
        CHECK(lexer.tokens()[1].length == 2);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 3);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 0);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("zeros padded") {
        const char* code = "0 | 0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kPipe);
        CHECK(lexer.tokens()[1].start == code + 2);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 4);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 0);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("zeros tight left") {
        const char* code = "0<< 0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kBinop);
        CHECK(lexer.tokens()[1].start == code + 1);
        CHECK(lexer.tokens()[1].length == 2);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 4);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 0);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("zeros tight right") {
        const char* code = "0 !@%&*<-+=|<>?/0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kBinop);
        CHECK(lexer.tokens()[1].start == code + 2);
        CHECK(lexer.tokens()[1].length == 14);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 16);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 0);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("chaining integers") {
        const char* code = "0!1/2 @ 0x3> 4 <5";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 11);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kBinop);
        CHECK(*(lexer.tokens()[1].start) == '!');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 2);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 1);
        CHECK(!lexer.tokens()[2].couldBeBinop);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kBinop);
        CHECK(*(lexer.tokens()[3].start) == '/');
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[3].couldBeBinop);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[4].start == code + 4);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[4].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[4].value.asInteger() == 2);
        CHECK(!lexer.tokens()[4].couldBeBinop);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kBinop);
        CHECK(*(lexer.tokens()[5].start) == '@');
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[5].couldBeBinop);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[6].start == code + 8);
        CHECK(lexer.tokens()[6].length == 3);
        CHECK(lexer.tokens()[6].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[6].value.asInteger() == 3);
        CHECK(!lexer.tokens()[6].couldBeBinop);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kGreaterThan);
        CHECK(*(lexer.tokens()[7].start) == '>');
        CHECK(lexer.tokens()[7].length == 1);
        CHECK(lexer.tokens()[7].couldBeBinop);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[8].start == code + 13);
        CHECK(lexer.tokens()[8].length == 1);
        CHECK(lexer.tokens()[8].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[8].value.asInteger() == 4);
        CHECK(!lexer.tokens()[8].couldBeBinop);
        CHECK(lexer.tokens()[9].type == Lexer::Token::Type::kLessThan);
        CHECK(*(lexer.tokens()[9].start) == '<');
        CHECK(lexer.tokens()[9].length == 1);
        CHECK(lexer.tokens()[9].couldBeBinop);
        CHECK(lexer.tokens()[10].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[10].start == code + 16);
        CHECK(lexer.tokens()[10].length == 1);
        CHECK(lexer.tokens()[10].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[10].value.asInteger() == 5);
        CHECK(!lexer.tokens()[10].couldBeBinop);
    }
    SUBCASE("strings tight") {
        const char* code = "\"a\"++\"bcdefg\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kString);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kBinop);
        CHECK(lexer.tokens()[1].start == code + 3);
        CHECK(lexer.tokens()[1].length == 2);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 6);
        CHECK(lexer.tokens()[2].length == 6);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kString);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("strings padded") {
        const char* code = "\"0123\" +/+ \"ABCD\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 4);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kString);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kBinop);
        CHECK(lexer.tokens()[1].start == code + 7);
        CHECK(lexer.tokens()[1].length == 3);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 12);
        CHECK(lexer.tokens()[2].length == 4);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kString);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("keyword binops") {
        const char* code = "a: x, b: y";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 5);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kKeyword);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[1].start == code + 3);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(!lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[2].start == code + 4);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(!lexer.tokens()[2].couldBeBinop);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kKeyword);
        CHECK(lexer.tokens()[3].start == code + 6);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[3].couldBeBinop);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[4].start == code + 9);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(!lexer.tokens()[4].couldBeBinop);
    }
}

TEST_CASE("Lexer Delimiters") {
    SUBCASE("all delims packed") {
        const char* code = "(){}[],;:^~#`";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 13);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kCloseParen);
        CHECK(lexer.tokens()[1].start == code + 1);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[2].start == code + 2);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[3].start == code + 3);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kOpenSquare);
        CHECK(lexer.tokens()[4].start == code + 4);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kCloseSquare);
        CHECK(lexer.tokens()[5].start == code + 5);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[6].start == code + 6);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kSemicolon);
        CHECK(lexer.tokens()[7].start == code + 7);
        CHECK(lexer.tokens()[7].length == 1);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kColon);
        CHECK(lexer.tokens()[8].start == code + 8);
        CHECK(lexer.tokens()[8].length == 1);
        CHECK(lexer.tokens()[9].type == Lexer::Token::Type::kCaret);
        CHECK(lexer.tokens()[9].start == code + 9);
        CHECK(lexer.tokens()[9].length == 1);
        CHECK(lexer.tokens()[10].type == Lexer::Token::Type::kTilde);
        CHECK(lexer.tokens()[10].start == code + 10);
        CHECK(lexer.tokens()[10].length == 1);
        CHECK(lexer.tokens()[11].type == Lexer::Token::Type::kHash);
        CHECK(lexer.tokens()[11].start == code + 11);
        CHECK(lexer.tokens()[11].length == 1);
        CHECK(lexer.tokens()[12].type == Lexer::Token::Type::kGrave);
        CHECK(lexer.tokens()[12].start == code + 12);
        CHECK(lexer.tokens()[12].length == 1);
    }
    SUBCASE("all delims loose") {
        const char* code = " ( ) { } [ ] , ; : ^ ~ # `";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 13);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kCloseParen);
        CHECK(lexer.tokens()[1].start == code + 3);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[2].start == code + 5);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[3].start == code + 7);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kOpenSquare);
        CHECK(lexer.tokens()[4].start == code + 9);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kCloseSquare);
        CHECK(lexer.tokens()[5].start == code + 11);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[6].start == code + 13);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kSemicolon);
        CHECK(lexer.tokens()[7].start == code + 15);
        CHECK(lexer.tokens()[7].length == 1);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kColon);
        CHECK(lexer.tokens()[8].start == code + 17);
        CHECK(lexer.tokens()[8].length == 1);
        CHECK(lexer.tokens()[9].type == Lexer::Token::Type::kCaret);
        CHECK(lexer.tokens()[9].start == code + 19);
        CHECK(lexer.tokens()[9].length == 1);
        CHECK(lexer.tokens()[10].type == Lexer::Token::Type::kTilde);
        CHECK(lexer.tokens()[10].start == code + 21);
        CHECK(lexer.tokens()[10].length == 1);
        CHECK(lexer.tokens()[11].type == Lexer::Token::Type::kHash);
        CHECK(lexer.tokens()[11].start == code + 23);
        CHECK(lexer.tokens()[11].length == 1);
        CHECK(lexer.tokens()[12].type == Lexer::Token::Type::kGrave);
        CHECK(lexer.tokens()[12].start == code + 25);
        CHECK(lexer.tokens()[12].length == 1);
    }
    SUBCASE("parens") {
        const char* code = ")((( ( ) ) (";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 8);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kCloseParen);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[1].start == code + 1);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[2].start == code + 2);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[3].start == code + 3);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[4].start == code + 5);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kCloseParen);
        CHECK(lexer.tokens()[5].start == code + 7);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kCloseParen);
        CHECK(lexer.tokens()[6].start == code + 9);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[7].start == code + 11);
        CHECK(lexer.tokens()[7].length == 1);
    }
    SUBCASE("mixed brackets") {
        const char* code = " { [ ( ({[]}) ) ] } ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 12);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[0].start == code + 1);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kOpenSquare);
        CHECK(lexer.tokens()[1].start == code + 3);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[2].start == code + 5);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[3].start == code + 7);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[4].start == code + 8);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kOpenSquare);
        CHECK(lexer.tokens()[5].start == code + 9);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kCloseSquare);
        CHECK(lexer.tokens()[6].start == code + 10);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[7].start == code + 11);
        CHECK(lexer.tokens()[7].length == 1);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kCloseParen);
        CHECK(lexer.tokens()[8].start == code + 12);
        CHECK(lexer.tokens()[8].length == 1);
        CHECK(lexer.tokens()[9].type == Lexer::Token::Type::kCloseParen);
        CHECK(lexer.tokens()[9].start == code + 14);
        CHECK(lexer.tokens()[9].length == 1);
        CHECK(lexer.tokens()[10].type == Lexer::Token::Type::kCloseSquare);
        CHECK(lexer.tokens()[10].start == code + 16);
        CHECK(lexer.tokens()[10].length == 1);
        CHECK(lexer.tokens()[11].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[11].start == code + 18);
        CHECK(lexer.tokens()[11].length == 1);
    }
    SUBCASE("heterogeneous array") {
        const char* code = "[\\a, [ 1, 0xe], [{000}, ( \"moof\") ], 'yea[h]',\";a:)_(<{}>,,]\" ]";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 23);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kOpenSquare);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[1].start == code + 2);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[1].value.type() == TypedLiteral::Type::kSymbol);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[2].start == code + 3);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kOpenSquare);
        CHECK(lexer.tokens()[3].start == code + 5);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[4].start == code + 7);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[4].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[4].value.asInteger() == 1);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[5].start == code + 8);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[6].start == code + 10);
        CHECK(lexer.tokens()[6].length == 3);
        CHECK(lexer.tokens()[6].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[6].value.asInteger() == 14);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kCloseSquare);
        CHECK(lexer.tokens()[7].start == code + 13);
        CHECK(lexer.tokens()[7].length == 1);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[8].start == code + 14);
        CHECK(lexer.tokens()[8].length == 1);
        CHECK(lexer.tokens()[9].type == Lexer::Token::Type::kOpenSquare);
        CHECK(lexer.tokens()[9].start == code + 16);
        CHECK(lexer.tokens()[9].length == 1);
        CHECK(lexer.tokens()[10].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[10].start == code + 17);
        CHECK(lexer.tokens()[10].length == 1);
        CHECK(lexer.tokens()[11].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[11].start == code + 18);
        CHECK(lexer.tokens()[11].length == 3);
        CHECK(lexer.tokens()[11].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[11].value.asInteger() == 0);
        CHECK(lexer.tokens()[12].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[12].start == code + 21);
        CHECK(lexer.tokens()[12].length == 1);
        CHECK(lexer.tokens()[13].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[13].start == code + 22);
        CHECK(lexer.tokens()[13].length == 1);
        CHECK(lexer.tokens()[14].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[14].start == code + 24);
        CHECK(lexer.tokens()[14].length == 1);
        CHECK(lexer.tokens()[15].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[15].start == code + 27);
        CHECK(lexer.tokens()[15].length == 4);
        CHECK(lexer.tokens()[15].value.type() == TypedLiteral::Type::kString);
        CHECK(lexer.tokens()[16].type == Lexer::Token::Type::kCloseParen);
        CHECK(lexer.tokens()[16].start == code + 32);
        CHECK(lexer.tokens()[16].length == 1);
        CHECK(lexer.tokens()[17].type == Lexer::Token::Type::kCloseSquare);
        CHECK(lexer.tokens()[17].start == code + 34);
        CHECK(lexer.tokens()[17].length == 1);
        CHECK(lexer.tokens()[18].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[18].start == code + 35);
        CHECK(lexer.tokens()[18].length == 1);
        CHECK(lexer.tokens()[19].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[19].start == code + 38);
        CHECK(lexer.tokens()[19].length == 6);
        CHECK(lexer.tokens()[19].value.type() == TypedLiteral::Type::kSymbol);
        CHECK(lexer.tokens()[20].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[20].start == code + 45);
        CHECK(lexer.tokens()[20].length == 1);
        CHECK(lexer.tokens()[21].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[21].start == code + 47);
        CHECK(lexer.tokens()[21].length == 13);
        CHECK(lexer.tokens()[21].value.type() == TypedLiteral::Type::kString);
        CHECK(lexer.tokens()[22].type == Lexer::Token::Type::kCloseSquare);
        CHECK(lexer.tokens()[22].start == code + 62);
        CHECK(lexer.tokens()[22].length == 1);
    }
}

TEST_CASE("Lexer Identifiers and Keywords") {
    SUBCASE("variable names") {
        const char* code = "x, abc_123_DEF ,nil_is_NOT_valid, argVarNilFalseTrue ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 7);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[1].start == code + 1);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[2].start == code + 3);
        CHECK(lexer.tokens()[2].length == 11);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[3].start == code + 15);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[4].start == code + 16);
        CHECK(lexer.tokens()[4].length == 16);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[5].start == code + 32);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[6].start == code + 34);
        CHECK(lexer.tokens()[6].length == 18);
    }
    SUBCASE("keywords") {
        const char* code = "var nil, arg true, false, const, classvar";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 11);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kVar);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[1].start == code + 4);
        CHECK(lexer.tokens()[1].length == 3);
        CHECK(lexer.tokens()[1].value.type() == TypedLiteral::Type::kNil);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[2].start == code + 7);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kArg);
        CHECK(lexer.tokens()[3].start == code + 9);
        CHECK(lexer.tokens()[3].length == 3);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[4].start == code + 13);
        CHECK(lexer.tokens()[4].length == 4);
        CHECK(lexer.tokens()[4].value.type() == TypedLiteral::Type::kBoolean);
        CHECK(lexer.tokens()[4].value.asBoolean());
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[5].start == code + 17);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[6].start == code + 19);
        CHECK(lexer.tokens()[6].length == 5);
        CHECK(lexer.tokens()[6].value.type() == TypedLiteral::Type::kBoolean);
        CHECK(!lexer.tokens()[6].value.asBoolean());
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[7].start == code + 24);
        CHECK(lexer.tokens()[7].length == 1);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kConst);
        CHECK(lexer.tokens()[8].start == code + 26);
        CHECK(lexer.tokens()[8].length == 5);
        CHECK(lexer.tokens()[9].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[9].start == code + 31);
        CHECK(lexer.tokens()[9].length == 1);
        CHECK(lexer.tokens()[10].type == Lexer::Token::Type::kClassVar);
        CHECK(lexer.tokens()[10].start == code + 33);
        CHECK(lexer.tokens()[10].length == 8);
    }
    SUBCASE("variable declarations") {
        const char* code = "var a, b17=23, cA = true,nil_ = \\asis;";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 15);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kVar);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[1].start == code + 4);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[2].start == code + 5);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[3].start == code + 7);
        CHECK(lexer.tokens()[3].length == 3);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kAssign);
        CHECK(lexer.tokens()[4].start == code + 10);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[5].start == code + 11);
        CHECK(lexer.tokens()[5].length == 2);
        CHECK(lexer.tokens()[5].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[5].value.asInteger() == 23);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[6].start == code + 13);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[7].start == code + 15);
        CHECK(lexer.tokens()[7].length == 2);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kAssign);
        CHECK(lexer.tokens()[8].start == code + 18);
        CHECK(lexer.tokens()[8].length == 1);
        CHECK(lexer.tokens()[9].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[9].start == code + 20);
        CHECK(lexer.tokens()[9].length == 4);
        CHECK(lexer.tokens()[9].value.type() == TypedLiteral::Type::kBoolean);
        CHECK(lexer.tokens()[9].value.asBoolean());
        CHECK(lexer.tokens()[10].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[10].start == code + 24);
        CHECK(lexer.tokens()[10].length == 1);
        CHECK(lexer.tokens()[11].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[11].start == code + 25);
        CHECK(lexer.tokens()[11].length == 4);
        CHECK(lexer.tokens()[12].type == Lexer::Token::Type::kAssign);
        CHECK(lexer.tokens()[12].start == code + 30);
        CHECK(lexer.tokens()[12].length == 1);
        CHECK(lexer.tokens()[13].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[13].start == code + 33);
        CHECK(lexer.tokens()[13].length == 4);
        CHECK(lexer.tokens()[13].value.type() == TypedLiteral::Type::kSymbol);
        CHECK(lexer.tokens()[14].type == Lexer::Token::Type::kSemicolon);
        CHECK(lexer.tokens()[14].start == code + 37);
        CHECK(lexer.tokens()[14].length == 1);
    }
    SUBCASE("argument list") {
        const char* code = "arg xyzyx,o4x,o=0x40 , k= \"nil;\";";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 13);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kArg);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[1].start == code + 4);
        CHECK(lexer.tokens()[1].length == 5);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[2].start == code + 9);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[3].start == code + 10);
        CHECK(lexer.tokens()[3].length == 3);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[4].start == code + 13);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[5].start == code + 14);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kAssign);
        CHECK(lexer.tokens()[6].start == code + 15);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[7].start == code + 16);
        CHECK(lexer.tokens()[7].length == 4);
        CHECK(lexer.tokens()[7].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[7].value.asInteger() == 0x40);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[8].start == code + 21);
        CHECK(lexer.tokens()[8].length == 1);
        CHECK(lexer.tokens()[9].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[9].start == code + 23);
        CHECK(lexer.tokens()[9].length == 1);
        CHECK(lexer.tokens()[10].type == Lexer::Token::Type::kAssign);
        CHECK(lexer.tokens()[10].start == code + 24);
        CHECK(lexer.tokens()[10].length == 1);
        CHECK(lexer.tokens()[11].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[11].start == code + 27);
        CHECK(lexer.tokens()[11].length == 4);
        CHECK(lexer.tokens()[11].value.type() == TypedLiteral::Type::kString);
        CHECK(lexer.tokens()[12].type == Lexer::Token::Type::kSemicolon);
        CHECK(lexer.tokens()[12].start == code + 32);
        CHECK(lexer.tokens()[12].length == 1);
    }
}

TEST_CASE("Lexer Class Names") {
    SUBCASE("definition") {
        const char* code = "X0_a { }B{}";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 6);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kClassName);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 4);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[1].start == code + 5);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[2].start == code + 7);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kClassName);
        CHECK(lexer.tokens()[3].start == code + 8);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[4].start == code + 9);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[5].start == code + 10);
        CHECK(lexer.tokens()[5].length == 1);
    }
    SUBCASE("inheritance") {
        const char* code = "Tu:V{}AMixedCaseClassName : SuperClass9000 { } ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 10);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kClassName);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 2);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kColon);
        CHECK(lexer.tokens()[1].start == code + 2);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kClassName);
        CHECK(lexer.tokens()[2].start == code + 3);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[3].start == code + 4);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[4].start == code + 5);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kClassName);
        CHECK(lexer.tokens()[5].start == code + 6);
        CHECK(lexer.tokens()[5].length == 19);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kColon);
        CHECK(lexer.tokens()[6].start == code + 26);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kClassName);
        CHECK(lexer.tokens()[7].start == code + 28);
        CHECK(lexer.tokens()[7].length == 14);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[8].start == code + 43);
        CHECK(lexer.tokens()[8].length == 1);
        CHECK(lexer.tokens()[9].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[9].start == code + 45);
        CHECK(lexer.tokens()[9].length == 1);
    }
    SUBCASE("extension") {
        const char* code = "+Object{} + Numb3r { }";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 8);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kPlus);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kClassName);
        CHECK(lexer.tokens()[1].start == code + 1);
        CHECK(lexer.tokens()[1].length == 6);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[2].start == code + 7);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[3].start == code + 8);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kPlus);
        CHECK(lexer.tokens()[4].start == code + 10);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kClassName);
        CHECK(lexer.tokens()[5].start == code + 12);
        CHECK(lexer.tokens()[5].length == 6);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[6].start == code + 19);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[7].start == code + 21);
        CHECK(lexer.tokens()[7].length == 1);
    }
    SUBCASE("method invocation") {
        const char* code = "Class.method(label: 4)";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 7);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kClassName);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 5);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kDot);
        CHECK(lexer.tokens()[1].start == code + 5);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[2].start == code + 6);
        CHECK(lexer.tokens()[2].length == 6);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[3].start == code + 12);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kKeyword);
        CHECK(lexer.tokens()[4].start == code + 13);
        CHECK(lexer.tokens()[4].length == 5);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[5].start == code + 20);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[5].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[5].value.asInteger() == 4);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kCloseParen);
        CHECK(lexer.tokens()[6].start == code + 21);
        CHECK(lexer.tokens()[6].length == 1);
    }
    SUBCASE("construction") {
        const char* code = "SynthDef(\\t, { SinOsc.ar(880) }).add;";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 16);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kClassName);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 8);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[1].start == code + 8);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 10);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kSymbol);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kComma);
        CHECK(lexer.tokens()[3].start == code + 11);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[4].start == code + 13);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kClassName);
        CHECK(lexer.tokens()[5].start == code + 15);
        CHECK(lexer.tokens()[5].length == 6);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kDot);
        CHECK(lexer.tokens()[6].start == code + 21);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[7].start == code + 22);
        CHECK(lexer.tokens()[7].length == 2);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kOpenParen);
        CHECK(lexer.tokens()[8].start == code + 24);
        CHECK(lexer.tokens()[8].length == 1);
        CHECK(lexer.tokens()[9].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[9].start == code + 25);
        CHECK(lexer.tokens()[9].length == 3);
        CHECK(lexer.tokens()[9].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[9].value.asInteger() == 880);
        CHECK(lexer.tokens()[10].type == Lexer::Token::Type::kCloseParen);
        CHECK(lexer.tokens()[10].start == code + 28);
        CHECK(lexer.tokens()[10].length == 1);
        CHECK(lexer.tokens()[11].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[11].start == code + 30);
        CHECK(lexer.tokens()[11].length == 1);
        CHECK(lexer.tokens()[12].type == Lexer::Token::Type::kCloseParen);
        CHECK(lexer.tokens()[12].start == code + 31);
        CHECK(lexer.tokens()[12].length == 1);
        CHECK(lexer.tokens()[13].type == Lexer::Token::Type::kDot);
        CHECK(lexer.tokens()[13].start == code + 32);
        CHECK(lexer.tokens()[13].length == 1);
        CHECK(lexer.tokens()[14].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[14].start == code + 33);
        CHECK(lexer.tokens()[14].length == 3);
        CHECK(lexer.tokens()[15].type == Lexer::Token::Type::kSemicolon);
        CHECK(lexer.tokens()[15].start == code + 36);
        CHECK(lexer.tokens()[15].length == 1);
    }
}

TEST_CASE("Lexer Dots") {
    SUBCASE("valid dot patterns") {
        const char* code = ". .. ...";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kDot);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kDotDot);
        CHECK(lexer.tokens()[1].start == code + 2);
        CHECK(lexer.tokens()[1].length == 2);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kEllipses);
        CHECK(lexer.tokens()[2].start == code + 5);
        CHECK(lexer.tokens()[2].length == 3);
    }
    SUBCASE("invalid dot pattern") {
        const char* code = "....";
        Lexer lexer(code);
        REQUIRE(!lexer.lex());
    }
    SUBCASE("method call") {
        const char* code = "a.ham";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kDot);
        CHECK(lexer.tokens()[1].start == code + 1);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[2].start == code + 2);
        CHECK(lexer.tokens()[2].length == 3);
    }
    SUBCASE("array slice") {
        const char* code = "xR[9..0]";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 6);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 2);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kOpenSquare);
        CHECK(lexer.tokens()[1].start == code + 2);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[2].start == code + 3);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 9);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kDotDot);
        CHECK(lexer.tokens()[3].start == code + 4);
        CHECK(lexer.tokens()[3].length == 2);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[4].start == code + 6);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[4].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[4].value.asInteger() == 0);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kCloseSquare);
        CHECK(lexer.tokens()[5].start == code + 7);
        CHECK(lexer.tokens()[5].length == 1);
    }
}

TEST_CASE("Lexer Comments") {
    SUBCASE("line comment unix line ending") {
        const char* code = "\t// line comment\n47";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 17);
        CHECK(lexer.tokens()[0].length == 2);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 47);
    }
    SUBCASE("line comment DOS line ending") {
        const char* code = "  // /* testing unterminated block \r\n  \"a\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code + 40);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kString);
    }
    SUBCASE("line comment extended chars") {
        const char* code = "// 寧為太平犬，不做亂世人\n";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 0);
    }
    SUBCASE("unterminated line comment") {
        const char* code = "// no newline at end";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 0);
    }
    SUBCASE("inline block comment") {
        const char* code = "var a = /* test comment */ x;";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 5);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kVar);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[1].start == code + 4);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kAssign);
        CHECK(lexer.tokens()[2].start == code + 6);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[3].start == code + 27);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kSemicolon);
        CHECK(lexer.tokens()[4].start == code + 28);
        CHECK(lexer.tokens()[4].length == 1);
    }
    SUBCASE("many star block comment") {
        const char* code = "/*********/";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 0);
    }
    SUBCASE("nested block comments allowed") {
        const char* code = "1 /* SuperCollider allows \n /* nested */ \n comments */ a";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kLiteral);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.type() == TypedLiteral::Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[1].start == code + 55);
        CHECK(lexer.tokens()[1].length == 1);
    }
    SUBCASE("block comment extended characters") {
        const char * code = "/* // ✌️a */";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 0);
    }
}

TEST_CASE("Lexer Primitives") {
    SUBCASE("raw primitive") {
        const char* code = "_Prim_A_B_C123";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kPrimitive);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 14);
    }

    SUBCASE("primitive in method") {
        const char* code = "A { m { |a| _Run_Secret_Code; } }";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 11);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kClassName);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[1].start == code + 2);
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[2].start == code + 4);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kOpenCurly);
        CHECK(lexer.tokens()[3].start == code + 6);
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kPipe);
        CHECK(lexer.tokens()[4].start == code + 8);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kIdentifier);
        CHECK(lexer.tokens()[5].start == code + 9);
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kPipe);
        CHECK(lexer.tokens()[6].start == code + 10);
        CHECK(lexer.tokens()[6].length == 1);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kPrimitive);
        CHECK(lexer.tokens()[7].start == code + 12);
        CHECK(lexer.tokens()[7].length == 16);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kSemicolon);
        CHECK(lexer.tokens()[8].start == code + 28);
        CHECK(lexer.tokens()[8].length == 1);
        CHECK(lexer.tokens()[9].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[9].start == code + 30);
        CHECK(lexer.tokens()[9].length == 1);
        CHECK(lexer.tokens()[10].type == Lexer::Token::Type::kCloseCurly);
        CHECK(lexer.tokens()[10].start == code + 32);
        CHECK(lexer.tokens()[10].length == 1);
    }
}

} // namespace hadron
