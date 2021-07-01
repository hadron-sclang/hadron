#include "Lexer.hpp"

#include "ErrorReporter.hpp"
#include "SymbolTable.hpp"

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
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
    }
    SUBCASE("zero-padded zero") {
        const char* code = "000";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
    }
    SUBCASE("whitespace-padded zero") {
        const char* code = "\n\t 0\r\t";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(*(lexer.tokens()[0].range.data()) == '0');
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
    }
    SUBCASE("single digit") {
        const char* code = "4";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 4);
    }
    SUBCASE("zero-padded single digit") {
        const char* code = "007";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 7);
    }
    SUBCASE("whitespace-padded single digit") {
        const char* code = "     9\t";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(*(lexer.tokens()[0].range.data()) == '9');
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 9);
    }
    SUBCASE("multi digit") {
        const char* code = "991157";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 6);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 991157);
    }
    SUBCASE("zero padded") {
        const char* code = "0000000000000000043";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 19);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 43);
    }
    SUBCASE("whitespace padded") {
        const char* code = "    869  ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 4);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 869);
    }
    SUBCASE("near 32 bit limit") {
        const char* code = "2147483647";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 10);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 2147483647);
    }
    SUBCASE("int list") {
        const char* code = "1,2, 3, 4";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 7);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 1);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[1].range.data() == code + 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 2);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 2);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[3].range.data() == code + 3);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[4].range.data() == code + 5);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[4].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[4].value.asInteger() == 3);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[5].range.data() == code + 6);
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[6].range.data() == code + 8);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[6].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[6].value.asInteger() == 4);
    }
    SUBCASE("int method call") {
        const char* code = "10.asString;";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 4);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 2);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 10);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kDot);
        CHECK(lexer.tokens()[1].range.data() == code + 2);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[2].range.data() == code + 3);
        CHECK(lexer.tokens()[2].range.size() == 8);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kSemicolon);
        CHECK(lexer.tokens()[3].range.data() == code + 11);
        CHECK(lexer.tokens()[3].range.size() == 1);
    }
}

TEST_CASE("Lexer Floating Point") {
    SUBCASE("float zero") {
        const char* code = "0.0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[0].value.type() == Type::kFloat);
        CHECK(lexer.tokens()[0].value.asFloat() == 0);
    }
    SUBCASE("leading zeros") {
        const char* code = "000.25";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 6);
        CHECK(lexer.tokens()[0].value.type() == Type::kFloat);
        CHECK(lexer.tokens()[0].value.asFloat() == 0.25);
    }
    SUBCASE("integer and fraction") {
        const char* code = "987.125";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 7);
        CHECK(lexer.tokens()[0].value.type() == Type::kFloat);
        CHECK(lexer.tokens()[0].value.asFloat() == 987.125);
    }
    SUBCASE("float method call") {
        const char* code = "1.23.asString";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 4);
        CHECK(lexer.tokens()[0].value.type() == Type::kFloat);
        CHECK(lexer.tokens()[0].value.asFloat() == 1.23f);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kDot);
        CHECK(lexer.tokens()[1].range.data() == code + 4);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[2].range.data() == code + 5);
        CHECK(lexer.tokens()[2].range.size() == 8);
    }
}

TEST_CASE("Lexer Hexadecimal Integers") {
    SUBCASE("zero") {
        const char* code = "0x0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
    }
    SUBCASE("zero elided <DIFFA1>") {
        const char* code = "0x";
        Lexer lexer(code);
        // Will lex as two tokens, an integer 0 and an identifier 'x'
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[1].range.data() == code + 1);
        CHECK(lexer.tokens()[1].range.size() == 1);
    }
    SUBCASE("single digit alpha") {
        const char* code = "0xa";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 10);
    }
    SUBCASE("single digit numeric") {
        const char* code = "0x2";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 2);
    }
    SUBCASE("multi-digit upper") {
        const char* code = "0xAAE724F";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 9);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0xAAE724F);
    }
    SUBCASE("multi-digit lower") {
        const char* code = "0xdeadb33f";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 10);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0xdeadb33f);
    }
    SUBCASE("multi-digit mixed") {
        const char* code = "0x1A2b3C";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 8);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0x1a2b3c);
    }
    SUBCASE("zero padding <DIFFA2>") {
        const char* code = "000x742a";
        Lexer lexer(code);
        // Lexer will lex "000" as an integer and "x742a" as an identifier.
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[1].range.data() == code + 3);
        CHECK(lexer.tokens()[1].range.size() == 5);
    }
    SUBCASE("nonzero padding <DIFFA2>") {
        const char* code = "12345x1";
        Lexer lexer(code);
        // Lexer will lex "12345" as an integer and "x1" as an identifier.
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 5);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 12345);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[1].range.data() == code + 5);
        CHECK(lexer.tokens()[1].range.size() == 2);
    }
    SUBCASE("whitespace padding") {
        const char* code = "    0x1234   ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 4);
        CHECK(lexer.tokens()[0].range.size() == 6);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0x1234);
    }
}

TEST_CASE("Lexer Strings") {
    SUBCASE("empty string") {
        const char* code = "\"\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 0);
        CHECK(lexer.tokens()[0].value.type() == Type::kString);
        CHECK(!lexer.tokens()[0].value.hasEscapeCharacters());
    }
    SUBCASE("simple string") {
        const char* code = "\"abc\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[0].value.type() == Type::kString);
        CHECK(!lexer.tokens()[0].value.hasEscapeCharacters());
    }
    SUBCASE("padded string") {
        const char* code = "  \"Spaces inside and out.\"  ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 3);
        CHECK(lexer.tokens()[0].range.size() == 22);
        CHECK(lexer.tokens()[0].value.type() == Type::kString);
        CHECK(!lexer.tokens()[0].value.hasEscapeCharacters());
    }
    SUBCASE("escape characters") {
        const char* code = "\"\t\n\r\\t\\r\\n\\\"0x'\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == std::strlen(code) - 2);
        CHECK(lexer.tokens()[0].value.type() == Type::kString);
        CHECK(lexer.tokens()[0].value.hasEscapeCharacters());
    }
    SUBCASE("adjacent strings tight") {
        const char* code = "\"a\"\"b\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kString);
        CHECK(!lexer.tokens()[0].value.hasEscapeCharacters());
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[1].range.data() == code + 4);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[1].value.type() == Type::kString);
        CHECK(!lexer.tokens()[1].value.hasEscapeCharacters());
    }
    SUBCASE("adjacent strings padded") {
        const char* code = "  \"\\\"\"  \"b\"  ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 3);
        CHECK(lexer.tokens()[0].range.size() == 2);
        CHECK(lexer.tokens()[0].value.type() == Type::kString);
        CHECK(lexer.tokens()[0].value.hasEscapeCharacters());
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[1].range.data() == code + 9);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[1].value.type() == Type::kString);
        CHECK(!lexer.tokens()[1].value.hasEscapeCharacters());
    }
    SUBCASE("extended characters in string") {
        const char* code = "\"(╯°□°)╯︵ ┻━┻\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == std::strlen(code) - 2);
        CHECK(lexer.tokens()[0].value.type() == Type::kString);
        CHECK(!lexer.tokens()[0].value.hasEscapeCharacters());
    }
    SUBCASE("unterminated string") {
        Lexer lexer("\"abc");
        CHECK(!lexer.lex());
    }
}

TEST_CASE("Lexer Symbols") {
    SUBCASE("empty quote symbol") {
        const char* code = "''";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 0);
        CHECK(lexer.tokens()[0].value.type() == Type::kSymbol);
        CHECK(!lexer.tokens()[0].value.hasEscapeCharacters());
    }
    SUBCASE("simple quote") {
        const char* code = "'bA1'";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[0].value.type() == Type::kSymbol);
        CHECK(!lexer.tokens()[0].value.hasEscapeCharacters());
    }
    SUBCASE("padded quote") {
        const char* code = "  'ALL CAPS READS LIKE SHOUTING'  ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 3);
        CHECK(lexer.tokens()[0].range.size() == 28);
        CHECK(lexer.tokens()[0].value.type() == Type::kSymbol);
        CHECK(!lexer.tokens()[0].value.hasEscapeCharacters());
    }
    SUBCASE("special characters") {
        const char* code = "'\\t\\n\\r\t\n\r\\'0x\"'";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 14);
        CHECK(lexer.tokens()[0].value.type() == Type::kSymbol);
        CHECK(lexer.tokens()[0].value.hasEscapeCharacters());
    }
    SUBCASE("unterminated quote") {
        Lexer lexer("'abc");
        CHECK(!lexer.lex());
    }
    SUBCASE("empty slash") {
        const char* code = "\\";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 0);
        CHECK(lexer.tokens()[0].value.type() == Type::kSymbol);
    }
    SUBCASE("empty slash with whitespace") {
        const char* code = "\\ ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 0);
        CHECK(lexer.tokens()[0].value.type() == Type::kSymbol);
        CHECK(!lexer.tokens()[0].value.hasEscapeCharacters());
    }
    SUBCASE("simple slash") {
        const char* code = "\\abcx_1234_ABCX";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 14);
        CHECK(lexer.tokens()[0].value.type() == Type::kSymbol);
        CHECK(!lexer.tokens()[0].value.hasEscapeCharacters());
    }
    SUBCASE("symbol sequence") {
        const char* code = "'A', \\b , 'c',\\D,'e'";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 9);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kSymbol);
        CHECK(!lexer.tokens()[0].value.hasEscapeCharacters());
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[1].range.data() == code + 3);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 6);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].value.type() == Type::kSymbol);
        CHECK(!lexer.tokens()[2].value.hasEscapeCharacters());
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[3].range.data() == code + 8);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[4].range.data() == code + 11);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[4].value.type() == Type::kSymbol);
        CHECK(!lexer.tokens()[4].value.hasEscapeCharacters());
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[5].range.data() == code + 13);
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[6].range.data() == code + 15);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[6].value.type() == Type::kSymbol);
        CHECK(!lexer.tokens()[6].value.hasEscapeCharacters());
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[7].range.size() == 1);
        CHECK(lexer.tokens()[7].range.data() == code + 16);
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[8].range.data() == code + 18);
        CHECK(lexer.tokens()[8].range.size() == 1);
        CHECK(lexer.tokens()[8].value.type() == Type::kSymbol);
        CHECK(!lexer.tokens()[8].value.hasEscapeCharacters());
    }
    SUBCASE("extended characters in quote symbols") {
        const char* code = "'🖤💛💙💜💚🧡'";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == std::strlen(code) - 2);
        CHECK(lexer.tokens()[0].value.type() == Type::kSymbol);
        CHECK(!lexer.tokens()[0].value.hasEscapeCharacters());
    }
}

TEST_CASE("Binary Operators") {
    SUBCASE("bare plus") {
        const char* code = "+ - * = < > | <> <-";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 9);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kPlus);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kMinus);
        CHECK(lexer.tokens()[1].range.data() == code + 2);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kAsterisk);
        CHECK(lexer.tokens()[2].range.data() == code + 4);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].couldBeBinop);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kAssign);
        CHECK(lexer.tokens()[3].range.data() == code + 6);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[3].couldBeBinop);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kLessThan);
        CHECK(lexer.tokens()[4].range.data() == code + 8);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[4].couldBeBinop);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kGreaterThan);
        CHECK(lexer.tokens()[5].range.data() == code + 10);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[5].couldBeBinop);
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kPipe);
        CHECK(lexer.tokens()[6].range.data() == code + 12);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[6].couldBeBinop);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kReadWriteVar);
        CHECK(lexer.tokens()[7].range.data() == code + 14);
        CHECK(lexer.tokens()[7].range.size() == 2);
        CHECK(lexer.tokens()[7].couldBeBinop);
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kLeftArrow);
        CHECK(lexer.tokens()[8].range.data() == code + 17);
        CHECK(lexer.tokens()[8].range.size() == 2);
        CHECK(lexer.tokens()[8].couldBeBinop);
    }
    SUBCASE("two integers padded") {
        const char* code = "1 + -22";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 4);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 1);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kPlus);
        CHECK(lexer.tokens()[1].range.data() == code + 2);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kMinus);
        CHECK(lexer.tokens()[2].range.data() == code + 4);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].couldBeBinop);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[3].range.data() == code + 5);
        CHECK(lexer.tokens()[3].range.size() == 2);
        CHECK(lexer.tokens()[3].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[3].value.asInteger() == 22);
        CHECK(!lexer.tokens()[3].couldBeBinop);
    }
    SUBCASE("two integers tight") {
        const char* code = "67!=4";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 2);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 67);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kBinop);
        CHECK(lexer.tokens()[1].range.data() == code + 2);
        CHECK(lexer.tokens()[1].range.size() == 2);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 4);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 4);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("tight left") {
        const char* code = "7+/+ 0x17";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 7);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kBinop);
        CHECK(lexer.tokens()[1].range.data() == code + 1);
        CHECK(lexer.tokens()[1].range.size() == 3);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 5);
        CHECK(lexer.tokens()[2].range.size() == 4);
        CHECK(lexer.tokens()[2].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 0x17);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("tight right") {
        const char* code = "0xffe *93";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 5);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0xffe);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kAsterisk);
        CHECK(*(lexer.tokens()[1].range.data()) == '*');
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 7);
        CHECK(lexer.tokens()[2].range.size() == 2);
        CHECK(lexer.tokens()[2].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 93);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("zeros tight") {
        const char* code = "0<-0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kLeftArrow);
        CHECK(lexer.tokens()[1].range.data() == code + 1);
        CHECK(lexer.tokens()[1].range.size() == 2);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 3);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 0);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("zeros padded") {
        const char* code = "0 | 0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kPipe);
        CHECK(lexer.tokens()[1].range.data() == code + 2);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 4);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 0);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("zeros tight left") {
        const char* code = "0<< 0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kBinop);
        CHECK(lexer.tokens()[1].range.data() == code + 1);
        CHECK(lexer.tokens()[1].range.size() == 2);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 4);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 0);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("zeros tight right") {
        const char* code = "0 !@%&*<-+=|<>?/0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kBinop);
        CHECK(lexer.tokens()[1].range.data() == code + 2);
        CHECK(lexer.tokens()[1].range.size() == 14);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[1].hash == SymbolTable::hash("!@%&*<-+=|<>?/"));
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 16);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 0);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("chaining integers") {
        const char* code = "0!1/2 @ 0x3> 4 <5";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 11);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 0);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kBinop);
        CHECK(*(lexer.tokens()[1].range.data()) == '!');
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[1].hash == SymbolTable::hash("!"));
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 2);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 1);
        CHECK(!lexer.tokens()[2].couldBeBinop);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kBinop);
        CHECK(*(lexer.tokens()[3].range.data()) == '/');
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[3].couldBeBinop);
        CHECK(lexer.tokens()[3].hash == SymbolTable::hash("/"));
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[4].range.data() == code + 4);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[4].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[4].value.asInteger() == 2);
        CHECK(!lexer.tokens()[4].couldBeBinop);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kBinop);
        CHECK(*(lexer.tokens()[5].range.data()) == '@');
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[5].couldBeBinop);
        CHECK(lexer.tokens()[5].hash == SymbolTable::hash("@"));
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[6].range.data() == code + 8);
        CHECK(lexer.tokens()[6].range.size() == 3);
        CHECK(lexer.tokens()[6].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[6].value.asInteger() == 3);
        CHECK(!lexer.tokens()[6].couldBeBinop);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kGreaterThan);
        CHECK(*(lexer.tokens()[7].range.data()) == '>');
        CHECK(lexer.tokens()[7].range.size() == 1);
        CHECK(lexer.tokens()[7].couldBeBinop);
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[8].range.data() == code + 13);
        CHECK(lexer.tokens()[8].range.size() == 1);
        CHECK(lexer.tokens()[8].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[8].value.asInteger() == 4);
        CHECK(!lexer.tokens()[8].couldBeBinop);
        CHECK(lexer.tokens()[9].name == Lexer::Token::Name::kLessThan);
        CHECK(*(lexer.tokens()[9].range.data()) == '<');
        CHECK(lexer.tokens()[9].range.size() == 1);
        CHECK(lexer.tokens()[9].couldBeBinop);
        CHECK(lexer.tokens()[10].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[10].range.data() == code + 16);
        CHECK(lexer.tokens()[10].range.size() == 1);
        CHECK(lexer.tokens()[10].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[10].value.asInteger() == 5);
        CHECK(!lexer.tokens()[10].couldBeBinop);
    }
    SUBCASE("strings tight") {
        const char* code = "\"a\"++\"bcdefg\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kString);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kBinop);
        CHECK(lexer.tokens()[1].range.data() == code + 3);
        CHECK(lexer.tokens()[1].range.size() == 2);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[1].hash == SymbolTable::hash("++"));
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 6);
        CHECK(lexer.tokens()[2].range.size() == 6);
        CHECK(lexer.tokens()[2].value.type() == Type::kString);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("strings padded") {
        const char* code = "\"0123\" +/+ \"ABCD\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 4);
        CHECK(lexer.tokens()[0].value.type() == Type::kString);
        CHECK(!lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kBinop);
        CHECK(lexer.tokens()[1].range.data() == code + 7);
        CHECK(lexer.tokens()[1].range.size() == 3);
        CHECK(lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 12);
        CHECK(lexer.tokens()[2].range.size() == 4);
        CHECK(lexer.tokens()[2].value.type() == Type::kString);
        CHECK(!lexer.tokens()[2].couldBeBinop);
    }
    SUBCASE("keyword binops") {
        const char* code = "a: x, b: y";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 5);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kKeyword);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].couldBeBinop);
        CHECK(lexer.tokens()[0].hash == SymbolTable::hash("a"));
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[1].range.data() == code + 3);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(!lexer.tokens()[1].couldBeBinop);
        CHECK(lexer.tokens()[1].hash == SymbolTable::hash("x"));
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[2].range.data() == code + 4);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(!lexer.tokens()[2].couldBeBinop);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kKeyword);
        CHECK(lexer.tokens()[3].range.data() == code + 6);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[3].couldBeBinop);
        CHECK(lexer.tokens()[3].hash == SymbolTable::hash("b"));
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[4].range.data() == code + 9);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(!lexer.tokens()[4].couldBeBinop);
        CHECK(lexer.tokens()[4].hash == SymbolTable::hash("y"));
    }
}

TEST_CASE("Lexer Delimiters") {
    SUBCASE("all delims packed") {
        const char* code = "(){}[],;:^~#`";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 13);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kCloseParen);
        CHECK(lexer.tokens()[1].range.data() == code + 1);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[2].range.data() == code + 2);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[3].range.data() == code + 3);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kOpenSquare);
        CHECK(lexer.tokens()[4].range.data() == code + 4);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kCloseSquare);
        CHECK(lexer.tokens()[5].range.data() == code + 5);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[6].range.data() == code + 6);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kSemicolon);
        CHECK(lexer.tokens()[7].range.data() == code + 7);
        CHECK(lexer.tokens()[7].range.size() == 1);
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kColon);
        CHECK(lexer.tokens()[8].range.data() == code + 8);
        CHECK(lexer.tokens()[8].range.size() == 1);
        CHECK(lexer.tokens()[9].name == Lexer::Token::Name::kCaret);
        CHECK(lexer.tokens()[9].range.data() == code + 9);
        CHECK(lexer.tokens()[9].range.size() == 1);
        CHECK(lexer.tokens()[10].name == Lexer::Token::Name::kTilde);
        CHECK(lexer.tokens()[10].range.data() == code + 10);
        CHECK(lexer.tokens()[10].range.size() == 1);
        CHECK(lexer.tokens()[11].name == Lexer::Token::Name::kHash);
        CHECK(lexer.tokens()[11].range.data() == code + 11);
        CHECK(lexer.tokens()[11].range.size() == 1);
        CHECK(lexer.tokens()[12].name == Lexer::Token::Name::kGrave);
        CHECK(lexer.tokens()[12].range.data() == code + 12);
        CHECK(lexer.tokens()[12].range.size() == 1);
    }
    SUBCASE("all delims loose") {
        const char* code = " ( ) { } [ ] , ; : ^ ~ # `";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 13);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kCloseParen);
        CHECK(lexer.tokens()[1].range.data() == code + 3);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[2].range.data() == code + 5);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[3].range.data() == code + 7);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kOpenSquare);
        CHECK(lexer.tokens()[4].range.data() == code + 9);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kCloseSquare);
        CHECK(lexer.tokens()[5].range.data() == code + 11);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[6].range.data() == code + 13);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kSemicolon);
        CHECK(lexer.tokens()[7].range.data() == code + 15);
        CHECK(lexer.tokens()[7].range.size() == 1);
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kColon);
        CHECK(lexer.tokens()[8].range.data() == code + 17);
        CHECK(lexer.tokens()[8].range.size() == 1);
        CHECK(lexer.tokens()[9].name == Lexer::Token::Name::kCaret);
        CHECK(lexer.tokens()[9].range.data() == code + 19);
        CHECK(lexer.tokens()[9].range.size() == 1);
        CHECK(lexer.tokens()[10].name == Lexer::Token::Name::kTilde);
        CHECK(lexer.tokens()[10].range.data() == code + 21);
        CHECK(lexer.tokens()[10].range.size() == 1);
        CHECK(lexer.tokens()[11].name == Lexer::Token::Name::kHash);
        CHECK(lexer.tokens()[11].range.data() == code + 23);
        CHECK(lexer.tokens()[11].range.size() == 1);
        CHECK(lexer.tokens()[12].name == Lexer::Token::Name::kGrave);
        CHECK(lexer.tokens()[12].range.data() == code + 25);
        CHECK(lexer.tokens()[12].range.size() == 1);
    }
    SUBCASE("parens") {
        const char* code = ")((( ( ) ) (";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 8);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kCloseParen);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[1].range.data() == code + 1);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[2].range.data() == code + 2);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[3].range.data() == code + 3);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[4].range.data() == code + 5);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kCloseParen);
        CHECK(lexer.tokens()[5].range.data() == code + 7);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kCloseParen);
        CHECK(lexer.tokens()[6].range.data() == code + 9);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[7].range.data() == code + 11);
        CHECK(lexer.tokens()[7].range.size() == 1);
    }
    SUBCASE("mixed brackets") {
        const char* code = " { [ ( ({[]}) ) ] } ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 12);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[0].range.data() == code + 1);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kOpenSquare);
        CHECK(lexer.tokens()[1].range.data() == code + 3);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[2].range.data() == code + 5);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[3].range.data() == code + 7);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[4].range.data() == code + 8);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kOpenSquare);
        CHECK(lexer.tokens()[5].range.data() == code + 9);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kCloseSquare);
        CHECK(lexer.tokens()[6].range.data() == code + 10);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[7].range.data() == code + 11);
        CHECK(lexer.tokens()[7].range.size() == 1);
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kCloseParen);
        CHECK(lexer.tokens()[8].range.data() == code + 12);
        CHECK(lexer.tokens()[8].range.size() == 1);
        CHECK(lexer.tokens()[9].name == Lexer::Token::Name::kCloseParen);
        CHECK(lexer.tokens()[9].range.data() == code + 14);
        CHECK(lexer.tokens()[9].range.size() == 1);
        CHECK(lexer.tokens()[10].name == Lexer::Token::Name::kCloseSquare);
        CHECK(lexer.tokens()[10].range.data() == code + 16);
        CHECK(lexer.tokens()[10].range.size() == 1);
        CHECK(lexer.tokens()[11].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[11].range.data() == code + 18);
        CHECK(lexer.tokens()[11].range.size() == 1);
    }
    SUBCASE("heterogeneous array") {
        const char* code = "[\\a, [ 1, 0xe], [{000}, ( \"moof\") ], 'yea[h]',\";a:)_(<{}>,,]\" ]";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 23);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kOpenSquare);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[1].range.data() == code + 2);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[1].value.type() == Type::kSymbol);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[2].range.data() == code + 3);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kOpenSquare);
        CHECK(lexer.tokens()[3].range.data() == code + 5);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[4].range.data() == code + 7);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[4].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[4].value.asInteger() == 1);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[5].range.data() == code + 8);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[6].range.data() == code + 10);
        CHECK(lexer.tokens()[6].range.size() == 3);
        CHECK(lexer.tokens()[6].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[6].value.asInteger() == 14);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kCloseSquare);
        CHECK(lexer.tokens()[7].range.data() == code + 13);
        CHECK(lexer.tokens()[7].range.size() == 1);
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[8].range.data() == code + 14);
        CHECK(lexer.tokens()[8].range.size() == 1);
        CHECK(lexer.tokens()[9].name == Lexer::Token::Name::kOpenSquare);
        CHECK(lexer.tokens()[9].range.data() == code + 16);
        CHECK(lexer.tokens()[9].range.size() == 1);
        CHECK(lexer.tokens()[10].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[10].range.data() == code + 17);
        CHECK(lexer.tokens()[10].range.size() == 1);
        CHECK(lexer.tokens()[11].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[11].range.data() == code + 18);
        CHECK(lexer.tokens()[11].range.size() == 3);
        CHECK(lexer.tokens()[11].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[11].value.asInteger() == 0);
        CHECK(lexer.tokens()[12].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[12].range.data() == code + 21);
        CHECK(lexer.tokens()[12].range.size() == 1);
        CHECK(lexer.tokens()[13].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[13].range.data() == code + 22);
        CHECK(lexer.tokens()[13].range.size() == 1);
        CHECK(lexer.tokens()[14].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[14].range.data() == code + 24);
        CHECK(lexer.tokens()[14].range.size() == 1);
        CHECK(lexer.tokens()[15].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[15].range.data() == code + 27);
        CHECK(lexer.tokens()[15].range.size() == 4);
        CHECK(lexer.tokens()[15].value.type() == Type::kString);
        CHECK(lexer.tokens()[16].name == Lexer::Token::Name::kCloseParen);
        CHECK(lexer.tokens()[16].range.data() == code + 32);
        CHECK(lexer.tokens()[16].range.size() == 1);
        CHECK(lexer.tokens()[17].name == Lexer::Token::Name::kCloseSquare);
        CHECK(lexer.tokens()[17].range.data() == code + 34);
        CHECK(lexer.tokens()[17].range.size() == 1);
        CHECK(lexer.tokens()[18].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[18].range.data() == code + 35);
        CHECK(lexer.tokens()[18].range.size() == 1);
        CHECK(lexer.tokens()[19].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[19].range.data() == code + 38);
        CHECK(lexer.tokens()[19].range.size() == 6);
        CHECK(lexer.tokens()[19].value.type() == Type::kSymbol);
        CHECK(lexer.tokens()[20].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[20].range.data() == code + 45);
        CHECK(lexer.tokens()[20].range.size() == 1);
        CHECK(lexer.tokens()[21].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[21].range.data() == code + 47);
        CHECK(lexer.tokens()[21].range.size() == 13);
        CHECK(lexer.tokens()[21].value.type() == Type::kString);
        CHECK(lexer.tokens()[22].name == Lexer::Token::Name::kCloseSquare);
        CHECK(lexer.tokens()[22].range.data() == code + 62);
        CHECK(lexer.tokens()[22].range.size() == 1);
    }
}

TEST_CASE("Lexer Identifiers and Keywords") {
    SUBCASE("variable names") {
        const char* code = "x, abc_123_DEF ,nil_is_NOT_valid, argVarNilFalseTrue ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 7);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].hash == SymbolTable::hash("x"));
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[1].range.data() == code + 1);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[2].range.data() == code + 3);
        CHECK(lexer.tokens()[2].range.size() == 11);
        CHECK(lexer.tokens()[2].hash == SymbolTable::hash("abc_123_DEF"));
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[3].range.data() == code + 15);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[4].range.data() == code + 16);
        CHECK(lexer.tokens()[4].range.size() == 16);
        CHECK(lexer.tokens()[4].hash == SymbolTable::hash("nil_is_NOT_valid"));
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[5].range.data() == code + 32);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[6].range.data() == code + 34);
        CHECK(lexer.tokens()[6].range.size() == 18);
        CHECK(lexer.tokens()[6].hash == SymbolTable::hash("argVarNilFalseTrue"));
    }
    SUBCASE("keywords") {
        const char* code = "var nil, arg true, false, const, classvar";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 11);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kVar);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[1].range.data() == code + 4);
        CHECK(lexer.tokens()[1].range.size() == 3);
        CHECK(lexer.tokens()[1].value.type() == Type::kNil);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[2].range.data() == code + 7);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kArg);
        CHECK(lexer.tokens()[3].range.data() == code + 9);
        CHECK(lexer.tokens()[3].range.size() == 3);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[4].range.data() == code + 13);
        CHECK(lexer.tokens()[4].range.size() == 4);
        CHECK(lexer.tokens()[4].value.type() == Type::kBoolean);
        CHECK(lexer.tokens()[4].value.asBoolean());
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[5].range.data() == code + 17);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[6].range.data() == code + 19);
        CHECK(lexer.tokens()[6].range.size() == 5);
        CHECK(lexer.tokens()[6].value.type() == Type::kBoolean);
        CHECK(!lexer.tokens()[6].value.asBoolean());
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[7].range.data() == code + 24);
        CHECK(lexer.tokens()[7].range.size() == 1);
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kConst);
        CHECK(lexer.tokens()[8].range.data() == code + 26);
        CHECK(lexer.tokens()[8].range.size() == 5);
        CHECK(lexer.tokens()[9].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[9].range.data() == code + 31);
        CHECK(lexer.tokens()[9].range.size() == 1);
        CHECK(lexer.tokens()[10].name == Lexer::Token::Name::kClassVar);
        CHECK(lexer.tokens()[10].range.data() == code + 33);
        CHECK(lexer.tokens()[10].range.size() == 8);
    }
    SUBCASE("variable declarations") {
        const char* code = "var a, b17=23, cA = true,nil_ = \\asis;";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 15);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kVar);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[1].range.data() == code + 4);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[1].hash == SymbolTable::hash("a"));
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[2].range.data() == code + 5);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[3].range.data() == code + 7);
        CHECK(lexer.tokens()[3].range.size() == 3);
        CHECK(lexer.tokens()[3].hash == SymbolTable::hash("b17"));
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kAssign);
        CHECK(lexer.tokens()[4].range.data() == code + 10);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[5].range.data() == code + 11);
        CHECK(lexer.tokens()[5].range.size() == 2);
        CHECK(lexer.tokens()[5].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[5].value.asInteger() == 23);
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[6].range.data() == code + 13);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[7].range.data() == code + 15);
        CHECK(lexer.tokens()[7].range.size() == 2);
        CHECK(lexer.tokens()[7].hash == SymbolTable::hash("cA"));
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kAssign);
        CHECK(lexer.tokens()[8].range.data() == code + 18);
        CHECK(lexer.tokens()[8].range.size() == 1);
        CHECK(lexer.tokens()[9].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[9].range.data() == code + 20);
        CHECK(lexer.tokens()[9].range.size() == 4);
        CHECK(lexer.tokens()[9].value.type() == Type::kBoolean);
        CHECK(lexer.tokens()[9].value.asBoolean());
        CHECK(lexer.tokens()[10].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[10].range.data() == code + 24);
        CHECK(lexer.tokens()[10].range.size() == 1);
        CHECK(lexer.tokens()[11].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[11].range.data() == code + 25);
        CHECK(lexer.tokens()[11].range.size() == 4);
        CHECK(lexer.tokens()[11].hash == SymbolTable::hash("nil_"));
        CHECK(lexer.tokens()[12].name == Lexer::Token::Name::kAssign);
        CHECK(lexer.tokens()[12].range.data() == code + 30);
        CHECK(lexer.tokens()[12].range.size() == 1);
        CHECK(lexer.tokens()[13].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[13].range.data() == code + 33);
        CHECK(lexer.tokens()[13].range.size() == 4);
        CHECK(lexer.tokens()[13].value.type() == Type::kSymbol);
        CHECK(lexer.tokens()[14].name == Lexer::Token::Name::kSemicolon);
        CHECK(lexer.tokens()[14].range.data() == code + 37);
        CHECK(lexer.tokens()[14].range.size() == 1);
    }
    SUBCASE("argument list") {
        const char* code = "arg xyzyx,o4x,o=0x40 , k= \"nil;\";";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 13);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kArg);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[1].range.data() == code + 4);
        CHECK(lexer.tokens()[1].range.size() == 5);
        CHECK(lexer.tokens()[1].hash == SymbolTable::hash("xyzyx"));
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[2].range.data() == code + 9);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[3].range.data() == code + 10);
        CHECK(lexer.tokens()[3].range.size() == 3);
        CHECK(lexer.tokens()[3].hash == SymbolTable::hash("o4x"));
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[4].range.data() == code + 13);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[5].range.data() == code + 14);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[5].hash == SymbolTable::hash("o"));
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kAssign);
        CHECK(lexer.tokens()[6].range.data() == code + 15);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[7].range.data() == code + 16);
        CHECK(lexer.tokens()[7].range.size() == 4);
        CHECK(lexer.tokens()[7].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[7].value.asInteger() == 0x40);
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[8].range.data() == code + 21);
        CHECK(lexer.tokens()[8].range.size() == 1);
        CHECK(lexer.tokens()[9].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[9].range.data() == code + 23);
        CHECK(lexer.tokens()[9].range.size() == 1);
        CHECK(lexer.tokens()[9].hash == SymbolTable::hash("k"));
        CHECK(lexer.tokens()[10].name == Lexer::Token::Name::kAssign);
        CHECK(lexer.tokens()[10].range.data() == code + 24);
        CHECK(lexer.tokens()[10].range.size() == 1);
        CHECK(lexer.tokens()[11].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[11].range.data() == code + 27);
        CHECK(lexer.tokens()[11].range.size() == 4);
        CHECK(lexer.tokens()[11].value.type() == Type::kString);
        CHECK(lexer.tokens()[12].name == Lexer::Token::Name::kSemicolon);
        CHECK(lexer.tokens()[12].range.data() == code + 32);
        CHECK(lexer.tokens()[12].range.size() == 1);
    }
}

TEST_CASE("Lexer Class Names") {
    SUBCASE("definition") {
        const char* code = "X0_a { }B{}";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 6);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kClassName);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 4);
        CHECK(lexer.tokens()[0].hash == SymbolTable::hash("X0_a"));
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[1].range.data() == code + 5);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[2].range.data() == code + 7);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kClassName);
        CHECK(lexer.tokens()[3].range.data() == code + 8);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[3].hash == SymbolTable::hash("B"));
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[4].range.data() == code + 9);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[5].range.data() == code + 10);
        CHECK(lexer.tokens()[5].range.size() == 1);
    }
    SUBCASE("inheritance") {
        const char* code = "Tu:V{}AMixedCaseClassName : SuperClass9000 { } ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 10);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kClassName);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 2);
        CHECK(lexer.tokens()[0].hash == SymbolTable::hash("Tu"));
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kColon);
        CHECK(lexer.tokens()[1].range.data() == code + 2);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kClassName);
        CHECK(lexer.tokens()[2].range.data() == code + 3);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].hash == SymbolTable::hash("V"));
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[3].range.data() == code + 4);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[4].range.data() == code + 5);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kClassName);
        CHECK(lexer.tokens()[5].range.data() == code + 6);
        CHECK(lexer.tokens()[5].range.size() == 19);
        CHECK(lexer.tokens()[5].hash == SymbolTable::hash("AMixedCaseClassName"));
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kColon);
        CHECK(lexer.tokens()[6].range.data() == code + 26);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kClassName);
        CHECK(lexer.tokens()[7].range.data() == code + 28);
        CHECK(lexer.tokens()[7].range.size() == 14);
        CHECK(lexer.tokens()[7].hash == SymbolTable::hash("SuperClass9000"));
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[8].range.data() == code + 43);
        CHECK(lexer.tokens()[8].range.size() == 1);
        CHECK(lexer.tokens()[9].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[9].range.data() == code + 45);
        CHECK(lexer.tokens()[9].range.size() == 1);
    }
    SUBCASE("extension") {
        const char* code = "+Object{} + Numb3r { }";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 8);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kPlus);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kClassName);
        CHECK(lexer.tokens()[1].range.data() == code + 1);
        CHECK(lexer.tokens()[1].range.size() == 6);
        CHECK(lexer.tokens()[1].hash == SymbolTable::hash("Object"));
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[2].range.data() == code + 7);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[3].range.data() == code + 8);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kPlus);
        CHECK(lexer.tokens()[4].range.data() == code + 10);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kClassName);
        CHECK(lexer.tokens()[5].range.data() == code + 12);
        CHECK(lexer.tokens()[5].range.size() == 6);
        CHECK(lexer.tokens()[5].hash == SymbolTable::hash("Numb3r"));
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[6].range.data() == code + 19);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[7].range.data() == code + 21);
        CHECK(lexer.tokens()[7].range.size() == 1);
    }
    SUBCASE("method invocation") {
        const char* code = "Class.method(label: 4)";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 7);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kClassName);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 5);
        CHECK(lexer.tokens()[0].hash == SymbolTable::hash("Class"));
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kDot);
        CHECK(lexer.tokens()[1].range.data() == code + 5);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[2].range.data() == code + 6);
        CHECK(lexer.tokens()[2].range.size() == 6);
        CHECK(lexer.tokens()[2].hash == SymbolTable::hash("method"));
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[3].range.data() == code + 12);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kKeyword);
        CHECK(lexer.tokens()[4].range.data() == code + 13);
        CHECK(lexer.tokens()[4].range.size() == 5);
        CHECK(lexer.tokens()[4].hash == SymbolTable::hash("label"));
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[5].range.data() == code + 20);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[5].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[5].value.asInteger() == 4);
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kCloseParen);
        CHECK(lexer.tokens()[6].range.data() == code + 21);
        CHECK(lexer.tokens()[6].range.size() == 1);
    }
    SUBCASE("construction") {
        const char* code = "SynthDef(\\t, { SinOsc.ar(880) }).add;";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 16);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kClassName);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 8);
        CHECK(lexer.tokens()[0].hash == SymbolTable::hash("SynthDef"));
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[1].range.data() == code + 8);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 10);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].value.type() == Type::kSymbol);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kComma);
        CHECK(lexer.tokens()[3].range.data() == code + 11);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[4].range.data() == code + 13);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kClassName);
        CHECK(lexer.tokens()[5].range.data() == code + 15);
        CHECK(lexer.tokens()[5].range.size() == 6);
        CHECK(lexer.tokens()[5].hash == SymbolTable::hash("SinOsc"));
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kDot);
        CHECK(lexer.tokens()[6].range.data() == code + 21);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[7].range.data() == code + 22);
        CHECK(lexer.tokens()[7].range.size() == 2);
        CHECK(lexer.tokens()[7].hash == SymbolTable::hash("ar"));
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kOpenParen);
        CHECK(lexer.tokens()[8].range.data() == code + 24);
        CHECK(lexer.tokens()[8].range.size() == 1);
        CHECK(lexer.tokens()[9].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[9].range.data() == code + 25);
        CHECK(lexer.tokens()[9].range.size() == 3);
        CHECK(lexer.tokens()[9].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[9].value.asInteger() == 880);
        CHECK(lexer.tokens()[10].name == Lexer::Token::Name::kCloseParen);
        CHECK(lexer.tokens()[10].range.data() == code + 28);
        CHECK(lexer.tokens()[10].range.size() == 1);
        CHECK(lexer.tokens()[11].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[11].range.data() == code + 30);
        CHECK(lexer.tokens()[11].range.size() == 1);
        CHECK(lexer.tokens()[12].name == Lexer::Token::Name::kCloseParen);
        CHECK(lexer.tokens()[12].range.data() == code + 31);
        CHECK(lexer.tokens()[12].range.size() == 1);
        CHECK(lexer.tokens()[13].name == Lexer::Token::Name::kDot);
        CHECK(lexer.tokens()[13].range.data() == code + 32);
        CHECK(lexer.tokens()[13].range.size() == 1);
        CHECK(lexer.tokens()[14].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[14].range.data() == code + 33);
        CHECK(lexer.tokens()[14].range.size() == 3);
        CHECK(lexer.tokens()[14].hash == SymbolTable::hash("add"));
        CHECK(lexer.tokens()[15].name == Lexer::Token::Name::kSemicolon);
        CHECK(lexer.tokens()[15].range.data() == code + 36);
        CHECK(lexer.tokens()[15].range.size() == 1);
    }
}

TEST_CASE("Lexer Dots") {
    SUBCASE("valid dot patterns") {
        const char* code = ". .. ...";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kDot);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kDotDot);
        CHECK(lexer.tokens()[1].range.data() == code + 2);
        CHECK(lexer.tokens()[1].range.size() == 2);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kEllipses);
        CHECK(lexer.tokens()[2].range.data() == code + 5);
        CHECK(lexer.tokens()[2].range.size() == 3);
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
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].hash == SymbolTable::hash("a"));
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kDot);
        CHECK(lexer.tokens()[1].range.data() == code + 1);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[2].range.data() == code + 2);
        CHECK(lexer.tokens()[2].range.size() == 3);
        CHECK(lexer.tokens()[2].hash == SymbolTable::hash("ham"));
    }
    SUBCASE("array slice") {
        const char* code = "xR[9..0]";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 6);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 2);
        CHECK(lexer.tokens()[0].hash == SymbolTable::hash("xR"));
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kOpenSquare);
        CHECK(lexer.tokens()[1].range.data() == code + 2);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[2].range.data() == code + 3);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[2].value.asInteger() == 9);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kDotDot);
        CHECK(lexer.tokens()[3].range.data() == code + 4);
        CHECK(lexer.tokens()[3].range.size() == 2);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[4].range.data() == code + 6);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[4].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[4].value.asInteger() == 0);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kCloseSquare);
        CHECK(lexer.tokens()[5].range.data() == code + 7);
        CHECK(lexer.tokens()[5].range.size() == 1);
    }
}

TEST_CASE("Lexer Comments") {
    SUBCASE("line comment unix line ending") {
        const char* code = "\t// line comment\n47";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 17);
        CHECK(lexer.tokens()[0].range.size() == 2);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 47);
    }
    SUBCASE("line comment DOS line ending") {
        const char* code = "  // /* testing unterminated block \r\n  \"a\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code + 40);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kString);
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
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kVar);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 3);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[1].range.data() == code + 4);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[1].hash == SymbolTable::hash("a"));
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kAssign);
        CHECK(lexer.tokens()[2].range.data() == code + 6);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[3].range.data() == code + 27);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[3].hash == SymbolTable::hash("x"));
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kSemicolon);
        CHECK(lexer.tokens()[4].range.data() == code + 28);
        CHECK(lexer.tokens()[4].range.size() == 1);
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
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kLiteral);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].value.type() == Type::kInteger);
        CHECK(lexer.tokens()[0].value.asInteger() == 1);
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[1].range.data() == code + 55);
        CHECK(lexer.tokens()[1].range.size() == 1);
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
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kPrimitive);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 14);
        CHECK(lexer.tokens()[0].hash == SymbolTable::hash("_Prim_A_B_C123"));
    }

    SUBCASE("primitive in method") {
        const char* code = "A { m { |a| _Run_Secret_Code; } }";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 11);
        CHECK(lexer.tokens()[0].name == Lexer::Token::Name::kClassName);
        CHECK(lexer.tokens()[0].range.data() == code);
        CHECK(lexer.tokens()[0].range.size() == 1);
        CHECK(lexer.tokens()[0].hash == SymbolTable::hash("A"));
        CHECK(lexer.tokens()[1].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[1].range.data() == code + 2);
        CHECK(lexer.tokens()[1].range.size() == 1);
        CHECK(lexer.tokens()[2].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[2].range.data() == code + 4);
        CHECK(lexer.tokens()[2].range.size() == 1);
        CHECK(lexer.tokens()[2].hash == SymbolTable::hash("m"));
        CHECK(lexer.tokens()[3].name == Lexer::Token::Name::kOpenCurly);
        CHECK(lexer.tokens()[3].range.data() == code + 6);
        CHECK(lexer.tokens()[3].range.size() == 1);
        CHECK(lexer.tokens()[4].name == Lexer::Token::Name::kPipe);
        CHECK(lexer.tokens()[4].range.data() == code + 8);
        CHECK(lexer.tokens()[4].range.size() == 1);
        CHECK(lexer.tokens()[5].name == Lexer::Token::Name::kIdentifier);
        CHECK(lexer.tokens()[5].range.data() == code + 9);
        CHECK(lexer.tokens()[5].range.size() == 1);
        CHECK(lexer.tokens()[5].hash == SymbolTable::hash("a"));
        CHECK(lexer.tokens()[6].name == Lexer::Token::Name::kPipe);
        CHECK(lexer.tokens()[6].range.data() == code + 10);
        CHECK(lexer.tokens()[6].range.size() == 1);
        CHECK(lexer.tokens()[7].name == Lexer::Token::Name::kPrimitive);
        CHECK(lexer.tokens()[7].range.data() == code + 12);
        CHECK(lexer.tokens()[7].range.size() == 16);
        CHECK(lexer.tokens()[7].hash == SymbolTable::hash("_Run_Secret_Code"));
        CHECK(lexer.tokens()[8].name == Lexer::Token::Name::kSemicolon);
        CHECK(lexer.tokens()[8].range.data() == code + 28);
        CHECK(lexer.tokens()[8].range.size() == 1);
        CHECK(lexer.tokens()[9].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[9].range.data() == code + 30);
        CHECK(lexer.tokens()[9].range.size() == 1);
        CHECK(lexer.tokens()[10].name == Lexer::Token::Name::kCloseCurly);
        CHECK(lexer.tokens()[10].range.data() == code + 32);
        CHECK(lexer.tokens()[10].range.size() == 1);
    }
}

} // namespace hadron
