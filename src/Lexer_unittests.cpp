#include "Lexer.hpp"

#include <doctest/doctest.h>

#include <vector>

namespace {

bool lex(hadron::Lexer& lexer, std::vector<hadron::Lexer::Token>& tokens) {
    while (lexer.next()) {
        tokens.emplace_back(lexer.token());
    }
    return !lexer.isError();
}

}

// TODO tests to add: 10.asString, 1.23.asString, "(╯°□°)╯︵ ┻━┻", -45.726, some other binops besides +

namespace hadron {

TEST_CASE("Lexer Base Cases") {
    SUBCASE("empty string") {
        std::vector<hadron::Lexer::Token> tokens;
        Lexer lexer("");
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 0);
    }
    SUBCASE("whitespace only") {
        std::vector<hadron::Lexer::Token> tokens;
        Lexer lexer("   \t\n\r  ");
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 0);
    }
}

TEST_CASE("Lexer Integers Simple") {
    SUBCASE("zero") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[0].value.integer == 0);
    }
    SUBCASE("zero-padded zero") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "000";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 3);
        CHECK(tokens[0].value.integer == 0);
    }
    SUBCASE("whitespace-padded zero") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "\n\t 0\r\t";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(*(tokens[0].start) == '0');
        CHECK(tokens[0].length == 1);
        CHECK(tokens[0].value.integer == 0);
    }
    SUBCASE("single digit") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "4";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[0].value.integer == 4);
    }
    SUBCASE("zero-padded single digit") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "007";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 3);
        CHECK(tokens[0].value.integer == 7);
    }
    SUBCASE("whitespace-padded single digit") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "     9\t";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(*(tokens[0].start) == '9');
        CHECK(tokens[0].length == 1);
        CHECK(tokens[0].value.integer == 9);
    }
    SUBCASE("multi digit") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "991157";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 6);
        CHECK(tokens[0].value.integer == 991157);
    }
    SUBCASE("zero padded") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0000000000000000043";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 19);
        CHECK(tokens[0].value.integer == 43);
    }
    SUBCASE("whitespace padded") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "    869  ";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code + 4);
        CHECK(tokens[0].length == 3);
        CHECK(tokens[0].value.integer == 869);
    }
    SUBCASE("near 32 bit limit") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "2147483647";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 10);
        CHECK(tokens[0].value.integer == 2147483647);
    }
    SUBCASE("above 32 bits <DIFFA0>") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "2147483648";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 10);
        CHECK(tokens[0].value.integer == 2147483648);
    }
    SUBCASE("int list") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "1,2, 3, 4";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 7);
    }
}

TEST_CASE("Lexer Floating Point") {
    SUBCASE("float zero") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0.0";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kFloat);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 3);
        CHECK(tokens[0].value.floatingPoint == 0);
    }
    SUBCASE("leading zeros") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "000.25";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kFloat);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 6);
        CHECK(tokens[0].value.floatingPoint == 0.25);
    }
    SUBCASE("integer and fraction") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "987.125";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kFloat);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 7);
        CHECK(tokens[0].value.floatingPoint == 987.125);
    }
}

TEST_CASE("Lexer Hexadecimal Integers") {
    SUBCASE("zero") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0x0";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 3);
        CHECK(tokens[0].value.integer == 0);
    }
    SUBCASE("zero elided <DIFFA1>") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0x";
        Lexer lexer(code);
        REQUIRE(!lex(lexer, tokens));
        REQUIRE(tokens.size() == 0);
    }
    SUBCASE("single digit alpha") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0xa";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 3);
        CHECK(tokens[0].value.integer == 10);
    }
    SUBCASE("single digit numeric") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0x2";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 3);
        CHECK(tokens[0].value.integer == 2);
    }
    SUBCASE("multi-digit upper") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0xAAE724F";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 9);
        CHECK(tokens[0].value.integer == 0xAAE724F);
    }
    SUBCASE("multi-digit lower") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0x42deadbeef42";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 14);
        CHECK(tokens[0].value.integer == 0x42deadbeef42);
    }
    SUBCASE("multi-digit mixed") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0x1A2b3C";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 8);
        CHECK(tokens[0].value.integer == 0x1a2b3c);
    }
    SUBCASE("zero padding") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "000x742a";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 8);
        CHECK(tokens[0].value.integer == 0x742a);
    }
    SUBCASE("whitespace padding") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "    0x1234   ";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code + 4);
        CHECK(tokens[0].length == 6);
        CHECK(tokens[0].value.integer == 0x1234);
    }
    SUBCASE("large value <DIFFA0>") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0x42deadbeef42";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 14);
        CHECK(tokens[0].value.integer == 0x42deadbeef42);
    }
}

TEST_CASE("Lexer Strings") {
    SUBCASE("empty string") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "\"\"";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kString);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 0);
    }
    SUBCASE("simple string") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "\"abc\"";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kString);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 3);
    }
    SUBCASE("padded string") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "  \"Spaces inside and out.\"  ";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kString);
        CHECK(tokens[0].start == code + 3);
        CHECK(tokens[0].length == 22);
    }
    SUBCASE("escape characters") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "\"\t\n\r\\t\\r\\n\\\"0x'\"";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kString);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 14);
    }
    SUBCASE("adjacent strings tight") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "\"a\"\"b\"";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 2);
        CHECK(tokens[0].type == Lexer::Token::Type::kString);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kString);
        CHECK(tokens[1].start == code + 4);
        CHECK(tokens[1].length == 1);
    }
    SUBCASE("adjacent strings padded") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "  \"\\\"\"  \"b\"  ";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 2);
        CHECK(tokens[0].type == Lexer::Token::Type::kString);
        CHECK(tokens[0].start == code + 3);
        CHECK(tokens[0].length == 2);
        CHECK(tokens[1].type == Lexer::Token::Type::kString);
        CHECK(tokens[1].start == code + 9);
        CHECK(tokens[1].length == 1);
    }
    SUBCASE("unterminated string") {
        std::vector<hadron::Lexer::Token> tokens;
        Lexer lexer("\"abc");
        REQUIRE(!lex(lexer, tokens));
    }
}

TEST_CASE("Lexer Symbols") {
    SUBCASE("empty quote symbol") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "''";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 0);
    }
    SUBCASE("simple quote") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "'bA1'";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 3);
    }
    SUBCASE("padded quote") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "  'ALL CAPS READS LIKE SHOUTING'  ";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[0].start == code + 3);
        CHECK(tokens[0].length == 28);
    }
    SUBCASE("special characters") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "'\\t\\n\\r\t\n\r\\'0x\"'";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 14);
    }
    SUBCASE("unterminated quote") {
        std::vector<hadron::Lexer::Token> tokens;
        Lexer lexer("'abc");
        REQUIRE(!lex(lexer, tokens));
    }
    SUBCASE("empty slash") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "\\";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 0);
    }
    SUBCASE("empty slash with whitespace") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "\\ ";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 0);
    }
    SUBCASE("simple slash") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "\\abcx_1234_ABCX";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 14);
    }
    SUBCASE("symbol sequence") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "'A', \\b , 'c',\\D,'e'";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 9);
        CHECK(tokens[0].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kComma);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[1].start == code + 3);
        CHECK(tokens[2].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[2].start == code + 6);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kComma);
        CHECK(tokens[3].length == 1);
        CHECK(tokens[3].start == code + 8);
        CHECK(tokens[4].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[4].start == code + 11);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[5].type == Lexer::Token::Type::kComma);
        CHECK(tokens[5].length == 1);
        CHECK(tokens[5].start == code + 13);
        CHECK(tokens[6].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[6].start == code + 15);
        CHECK(tokens[6].length == 1);
        CHECK(tokens[7].type == Lexer::Token::Type::kComma);
        CHECK(tokens[7].length == 1);
        CHECK(tokens[7].start == code + 16);
        CHECK(tokens[8].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[8].start == code + 18);
        CHECK(tokens[8].length == 1);
    }
}

TEST_CASE("Lexer Addition") {
    SUBCASE("bare plus") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "+";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].type == Lexer::Token::Type::kPlus);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
    }
    SUBCASE("two integers padded") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "1 + 22";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[0].value.integer == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[1].start) == '+');
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[2].start == code + 4);
        CHECK(tokens[2].length == 2);
        CHECK(tokens[2].value.integer == 22);
    }
    SUBCASE("two integers tight") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "67+4";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 2);
        CHECK(tokens[0].value.integer == 67);
        CHECK(tokens[1].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[1].start) == '+');
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[2].start == code + 3);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[2].value.integer == 4);
    }
    SUBCASE("tight left") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "7+ 0x17";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[0].value.integer == 7);
        CHECK(tokens[1].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[1].start) == '+');
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[2].start == code + 3);
        CHECK(tokens[2].length == 4);
        CHECK(tokens[2].value.integer == 0x17);
    }
    SUBCASE("tight right") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0xffe +93";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 5);
        CHECK(tokens[0].value.integer == 0xffe);
        CHECK(tokens[1].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[1].start) == '+');
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[2].start == code + 7);
        CHECK(tokens[2].length == 2);
        CHECK(tokens[2].value.integer == 93);
    }
    SUBCASE("zeros tight") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0+0";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[0].value.integer == 0);
        CHECK(tokens[1].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[1].start) == '+');
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[2].start == code + 2);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[2].value.integer == 0);
    }
    SUBCASE("zeros padded") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0 + 0";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[0].value.integer == 0);
        CHECK(tokens[1].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[1].start) == '+');
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[2].start == code + 4);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[2].value.integer == 0);
    }
    SUBCASE("zeros tight left") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0+ 0";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[0].value.integer == 0);
        CHECK(tokens[1].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[1].start) == '+');
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[2].start == code + 3);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[2].value.integer == 0);
    }
    SUBCASE("zeros tight right") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0 +0";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[0].value.integer == 0);
        CHECK(tokens[1].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[1].start) == '+');
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[2].start == code + 3);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[2].value.integer == 0);
    }
    SUBCASE("chaining integers") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "0+1+2 + 0x3+ 4 +5";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 11);
        CHECK(tokens[0].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[0].value.integer == 0);
        CHECK(tokens[1].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[1].start) == '+');
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[2].start == code + 2);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[2].value.integer == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[3].start) == '+');
        CHECK(tokens[3].length == 1);
        CHECK(tokens[4].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[4].start == code + 4);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[4].value.integer == 2);
        CHECK(tokens[5].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[5].start) == '+');
        CHECK(tokens[5].length == 1);
        CHECK(tokens[6].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[6].start == code + 8);
        CHECK(tokens[6].length == 3);
        CHECK(tokens[6].value.integer == 3);
        CHECK(tokens[7].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[7].start) == '+');
        CHECK(tokens[7].length == 1);
        CHECK(tokens[8].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[8].start == code + 13);
        CHECK(tokens[8].length == 1);
        CHECK(tokens[8].value.integer == 4);
        CHECK(tokens[9].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[9].start) == '+');
        CHECK(tokens[9].length == 1);
        CHECK(tokens[10].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[10].start == code + 16);
        CHECK(tokens[10].length == 1);
        CHECK(tokens[10].value.integer == 5);
    }
    SUBCASE("strings tight") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "\"a\"+\"bcdefg\"";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0].type == Lexer::Token::Type::kString);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[1].start) == '+');
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kString);
        CHECK(tokens[2].start == code + 5);
        CHECK(tokens[2].length == 6);
    }
    SUBCASE("strings padded") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "\"0123\" + \"ABCD\"";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0].type == Lexer::Token::Type::kString);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 4);
        CHECK(tokens[1].type == Lexer::Token::Type::kPlus);
        CHECK(*(tokens[1].start) == '+');
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kString);
        CHECK(tokens[2].start == code + 10);
        CHECK(tokens[2].length == 4);
    }
}

TEST_CASE("Lexer Delimiters") {
    SUBCASE("all delims packed") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "(){}[],;:^~";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 11);
        CHECK(tokens[0].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kCloseParen);
        CHECK(tokens[1].start == code + 1);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kOpenCurly);
        CHECK(tokens[2].start == code + 2);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kCloseCurly);
        CHECK(tokens[3].start == code + 3);
        CHECK(tokens[3].length == 1);
        CHECK(tokens[4].type == Lexer::Token::Type::kOpenSquare);
        CHECK(tokens[4].start == code + 4);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[5].type == Lexer::Token::Type::kCloseSquare);
        CHECK(tokens[5].start == code + 5);
        CHECK(tokens[5].length == 1);
        CHECK(tokens[6].type == Lexer::Token::Type::kComma);
        CHECK(tokens[6].start == code + 6);
        CHECK(tokens[6].length == 1);
        CHECK(tokens[7].type == Lexer::Token::Type::kSemicolon);
        CHECK(tokens[7].start == code + 7);
        CHECK(tokens[7].length == 1);
        CHECK(tokens[8].type == Lexer::Token::Type::kColon);
        CHECK(tokens[8].start == code + 8);
        CHECK(tokens[8].length == 1);
        CHECK(tokens[9].type == Lexer::Token::Type::kCaret);
        CHECK(tokens[9].start == code + 9);
        CHECK(tokens[9].length == 1);
        CHECK(tokens[10].type == Lexer::Token::Type::kTilde);
        CHECK(tokens[10].start == code + 10);
        CHECK(tokens[10].length == 1);
    }
    SUBCASE("all delims loose") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = " ( ) { } [ ] , ; : ^ ~ ";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 11);
        CHECK(tokens[0].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kCloseParen);
        CHECK(tokens[1].start == code + 3);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kOpenCurly);
        CHECK(tokens[2].start == code + 5);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kCloseCurly);
        CHECK(tokens[3].start == code + 7);
        CHECK(tokens[3].length == 1);
        CHECK(tokens[4].type == Lexer::Token::Type::kOpenSquare);
        CHECK(tokens[4].start == code + 9);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[5].type == Lexer::Token::Type::kCloseSquare);
        CHECK(tokens[5].start == code + 11);
        CHECK(tokens[5].length == 1);
        CHECK(tokens[6].type == Lexer::Token::Type::kComma);
        CHECK(tokens[6].start == code + 13);
        CHECK(tokens[6].length == 1);
        CHECK(tokens[7].type == Lexer::Token::Type::kSemicolon);
        CHECK(tokens[7].start == code + 15);
        CHECK(tokens[7].length == 1);
        CHECK(tokens[8].type == Lexer::Token::Type::kColon);
        CHECK(tokens[8].start == code + 17);
        CHECK(tokens[8].length == 1);
        CHECK(tokens[9].type == Lexer::Token::Type::kCaret);
        CHECK(tokens[9].start == code + 19);
        CHECK(tokens[9].length == 1);
        CHECK(tokens[10].type == Lexer::Token::Type::kTilde);
        CHECK(tokens[10].start == code + 21);
        CHECK(tokens[10].length == 1);
    }
    SUBCASE("parens") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = ")((( ( ) ) (";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 8);
        CHECK(tokens[0].type == Lexer::Token::Type::kCloseParen);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[1].start == code + 1);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[2].start == code + 2);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[3].start == code + 3);
        CHECK(tokens[3].length == 1);
        CHECK(tokens[4].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[4].start == code + 5);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[5].type == Lexer::Token::Type::kCloseParen);
        CHECK(tokens[5].start == code + 7);
        CHECK(tokens[5].length == 1);
        CHECK(tokens[6].type == Lexer::Token::Type::kCloseParen);
        CHECK(tokens[6].start == code + 9);
        CHECK(tokens[6].length == 1);
        CHECK(tokens[7].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[7].start == code + 11);
        CHECK(tokens[7].length == 1);
    }
    SUBCASE("mixed brackets") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = " { [ ( ({[]}) ) ] } ";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 12);
        CHECK(tokens[0].type == Lexer::Token::Type::kOpenCurly);
        CHECK(tokens[0].start == code + 1);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kOpenSquare);
        CHECK(tokens[1].start == code + 3);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[2].start == code + 5);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[3].start == code + 7);
        CHECK(tokens[3].length == 1);
        CHECK(tokens[4].type == Lexer::Token::Type::kOpenCurly);
        CHECK(tokens[4].start == code + 8);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[5].type == Lexer::Token::Type::kOpenSquare);
        CHECK(tokens[5].start == code + 9);
        CHECK(tokens[5].length == 1);
        CHECK(tokens[6].type == Lexer::Token::Type::kCloseSquare);
        CHECK(tokens[6].start == code + 10);
        CHECK(tokens[6].length == 1);
        CHECK(tokens[7].type == Lexer::Token::Type::kCloseCurly);
        CHECK(tokens[7].start == code + 11);
        CHECK(tokens[7].length == 1);
        CHECK(tokens[8].type == Lexer::Token::Type::kCloseParen);
        CHECK(tokens[8].start == code + 12);
        CHECK(tokens[8].length == 1);
        CHECK(tokens[9].type == Lexer::Token::Type::kCloseParen);
        CHECK(tokens[9].start == code + 14);
        CHECK(tokens[9].length == 1);
        CHECK(tokens[10].type == Lexer::Token::Type::kCloseSquare);
        CHECK(tokens[10].start == code + 16);
        CHECK(tokens[10].length == 1);
        CHECK(tokens[11].type == Lexer::Token::Type::kCloseCurly);
        CHECK(tokens[11].start == code + 18);
        CHECK(tokens[11].length == 1);
    }
    SUBCASE("heterogeneous array") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "[\\a, [ 1, 0xe], [{000}, ( \"moof\") ], 'yea[h]',\";a:)_(<{}>,,]\" ]";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 23);
        CHECK(tokens[0].type == Lexer::Token::Type::kOpenSquare);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[1].start == code + 2);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kComma);
        CHECK(tokens[2].start == code + 3);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kOpenSquare);
        CHECK(tokens[3].start == code + 5);
        CHECK(tokens[3].length == 1);
        CHECK(tokens[4].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[4].start == code + 7);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[4].value.integer == 1);
        CHECK(tokens[5].type == Lexer::Token::Type::kComma);
        CHECK(tokens[5].start == code + 8);
        CHECK(tokens[5].length == 1);
        CHECK(tokens[6].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[6].start == code + 10);
        CHECK(tokens[6].length == 3);
        CHECK(tokens[6].value.integer == 14);
        CHECK(tokens[7].type == Lexer::Token::Type::kCloseSquare);
        CHECK(tokens[7].start == code + 13);
        CHECK(tokens[7].length == 1);
        CHECK(tokens[8].type == Lexer::Token::Type::kComma);
        CHECK(tokens[8].start == code + 14);
        CHECK(tokens[8].length == 1);
        CHECK(tokens[9].type == Lexer::Token::Type::kOpenSquare);
        CHECK(tokens[9].start == code + 16);
        CHECK(tokens[9].length == 1);
        CHECK(tokens[10].type == Lexer::Token::Type::kOpenCurly);
        CHECK(tokens[10].start == code + 17);
        CHECK(tokens[10].length == 1);
        CHECK(tokens[11].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[11].start == code + 18);
        CHECK(tokens[11].length == 3);
        CHECK(tokens[11].value.integer == 0);
        CHECK(tokens[12].type == Lexer::Token::Type::kCloseCurly);
        CHECK(tokens[12].start == code + 21);
        CHECK(tokens[12].length == 1);
        CHECK(tokens[13].type == Lexer::Token::Type::kComma);
        CHECK(tokens[13].start == code + 22);
        CHECK(tokens[13].length == 1);
        CHECK(tokens[14].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[14].start == code + 24);
        CHECK(tokens[14].length == 1);
        CHECK(tokens[15].type == Lexer::Token::Type::kString);
        CHECK(tokens[15].start == code + 27);
        CHECK(tokens[15].length == 4);
        CHECK(tokens[16].type == Lexer::Token::Type::kCloseParen);
        CHECK(tokens[16].start == code + 32);
        CHECK(tokens[16].length == 1);
        CHECK(tokens[17].type == Lexer::Token::Type::kCloseSquare);
        CHECK(tokens[17].start == code + 34);
        CHECK(tokens[17].length == 1);
        CHECK(tokens[18].type == Lexer::Token::Type::kComma);
        CHECK(tokens[18].start == code + 35);
        CHECK(tokens[18].length == 1);
        CHECK(tokens[19].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[19].start == code + 38);
        CHECK(tokens[19].length == 6);
        CHECK(tokens[20].type == Lexer::Token::Type::kComma);
        CHECK(tokens[20].start == code + 45);
        CHECK(tokens[20].length == 1);
        CHECK(tokens[21].type == Lexer::Token::Type::kString);
        CHECK(tokens[21].start == code + 47);
        CHECK(tokens[21].length == 13);
        CHECK(tokens[22].type == Lexer::Token::Type::kCloseSquare);
        CHECK(tokens[22].start == code + 62);
        CHECK(tokens[22].length == 1);
    }
}

TEST_CASE("Lexer Identifiers") {
    SUBCASE("variable names") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "x, abc_123_DEF ,nil_is_NOT_valid, argVarNilFalseTrue ";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 7);
        CHECK(tokens[0].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kComma);
        CHECK(tokens[1].start == code + 1);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[2].start == code + 3);
        CHECK(tokens[2].length == 11);
        CHECK(tokens[3].type == Lexer::Token::Type::kComma);
        CHECK(tokens[3].start == code + 15);
        CHECK(tokens[3].length == 1);
        CHECK(tokens[4].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[4].start == code + 16);
        CHECK(tokens[4].length == 16);
        CHECK(tokens[5].type == Lexer::Token::Type::kComma);
        CHECK(tokens[5].start == code + 32);
        CHECK(tokens[5].length == 1);
        CHECK(tokens[6].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[6].start == code + 34);
        CHECK(tokens[6].length == 18);
    }
    SUBCASE("keywords") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "var nil, arg true, false";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 7);
        CHECK(tokens[0].type == Lexer::Token::Type::kVar);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 3);
        CHECK(tokens[1].type == Lexer::Token::Type::kNil);
        CHECK(tokens[1].start == code + 4);
        CHECK(tokens[1].length == 3);
        CHECK(tokens[2].type == Lexer::Token::Type::kComma);
        CHECK(tokens[2].start == code + 7);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kArg);
        CHECK(tokens[3].start == code + 9);
        CHECK(tokens[3].length == 3);
        CHECK(tokens[4].type == Lexer::Token::Type::kTrue);
        CHECK(tokens[4].start == code + 13);
        CHECK(tokens[4].length == 4);
        CHECK(tokens[5].type == Lexer::Token::Type::kComma);
        CHECK(tokens[5].start == code + 17);
        CHECK(tokens[5].length == 1);
        CHECK(tokens[6].type == Lexer::Token::Type::kFalse);
        CHECK(tokens[6].start == code + 19);
        CHECK(tokens[6].length == 5);
    }
    SUBCASE("variable declarations") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "var a, b17=23, cA = true,nil_ = \\asis;";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 15);
        CHECK(tokens[0].type == Lexer::Token::Type::kVar);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 3);
        CHECK(tokens[1].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[1].start == code + 4);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kComma);
        CHECK(tokens[2].start == code + 5);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[3].start == code + 7);
        CHECK(tokens[3].length == 3);
        CHECK(tokens[4].type == Lexer::Token::Type::kAssign);
        CHECK(tokens[4].start == code + 10);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[5].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[5].start == code + 11);
        CHECK(tokens[5].length == 2);
        CHECK(tokens[5].value.integer == 23);
        CHECK(tokens[6].type == Lexer::Token::Type::kComma);
        CHECK(tokens[6].start == code + 13);
        CHECK(tokens[6].length == 1);
        CHECK(tokens[7].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[7].start == code + 15);
        CHECK(tokens[7].length == 2);
        CHECK(tokens[8].type == Lexer::Token::Type::kAssign);
        CHECK(tokens[8].start == code + 18);
        CHECK(tokens[8].length == 1);
        CHECK(tokens[9].type == Lexer::Token::Type::kTrue);
        CHECK(tokens[9].start == code + 20);
        CHECK(tokens[9].length == 4);
        CHECK(tokens[10].type == Lexer::Token::Type::kComma);
        CHECK(tokens[10].start == code + 24);
        CHECK(tokens[10].length == 1);
        CHECK(tokens[11].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[11].start == code + 25);
        CHECK(tokens[11].length == 4);
        CHECK(tokens[12].type == Lexer::Token::Type::kAssign);
        CHECK(tokens[12].start == code + 30);
        CHECK(tokens[12].length == 1);
        CHECK(tokens[13].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[13].start == code + 33);
        CHECK(tokens[13].length == 4);
        CHECK(tokens[14].type == Lexer::Token::Type::kSemicolon);
        CHECK(tokens[14].start == code + 37);
        CHECK(tokens[14].length == 1);
    }
    SUBCASE("argument list") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "arg xyzyx,o4x,o=0x40 , k= \"nil;\";";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 13);
        CHECK(tokens[0].type == Lexer::Token::Type::kArg);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 3);
        CHECK(tokens[1].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[1].start == code + 4);
        CHECK(tokens[1].length == 5);
        CHECK(tokens[2].type == Lexer::Token::Type::kComma);
        CHECK(tokens[2].start == code + 9);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[3].start == code + 10);
        CHECK(tokens[3].length == 3);
        CHECK(tokens[4].type == Lexer::Token::Type::kComma);
        CHECK(tokens[4].start == code + 13);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[5].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[5].start == code + 14);
        CHECK(tokens[5].length == 1);
        CHECK(tokens[6].type == Lexer::Token::Type::kAssign);
        CHECK(tokens[6].start == code + 15);
        CHECK(tokens[6].length == 1);
        CHECK(tokens[7].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[7].start == code + 16);
        CHECK(tokens[7].length == 4);
        CHECK(tokens[7].value.integer == 0x40);
        CHECK(tokens[8].type == Lexer::Token::Type::kComma);
        CHECK(tokens[8].start == code + 21);
        CHECK(tokens[8].length == 1);
        CHECK(tokens[9].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[9].start == code + 23);
        CHECK(tokens[9].length == 1);
        CHECK(tokens[10].type == Lexer::Token::Type::kAssign);
        CHECK(tokens[10].start == code + 24);
        CHECK(tokens[10].length == 1);
        CHECK(tokens[11].type == Lexer::Token::Type::kString);
        CHECK(tokens[11].start == code + 27);
        CHECK(tokens[11].length == 4);
        CHECK(tokens[12].type == Lexer::Token::Type::kSemicolon);
        CHECK(tokens[12].start == code + 32);
        CHECK(tokens[12].length == 1);
    }
}

TEST_CASE("Lexer Class Names") {
    SUBCASE("definition") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "X0_a { }B{}";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 6);
        CHECK(tokens[0].type == Lexer::Token::Type::kClassName);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 4);
        CHECK(tokens[1].type == Lexer::Token::Type::kOpenCurly);
        CHECK(tokens[1].start == code + 5);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kCloseCurly);
        CHECK(tokens[2].start == code + 7);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kClassName);
        CHECK(tokens[3].start == code + 8);
        CHECK(tokens[3].length == 1);
        CHECK(tokens[4].type == Lexer::Token::Type::kOpenCurly);
        CHECK(tokens[4].start == code + 9);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[5].type == Lexer::Token::Type::kCloseCurly);
        CHECK(tokens[5].start == code + 10);
        CHECK(tokens[5].length == 1);
    }
    SUBCASE("inheritance") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "Tu:V{}AMixedCaseClassName : SuperClass9000 { } ";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 10);
        CHECK(tokens[0].type == Lexer::Token::Type::kClassName);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 2);
        CHECK(tokens[1].type == Lexer::Token::Type::kColon);
        CHECK(tokens[1].start == code + 2);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kClassName);
        CHECK(tokens[2].start == code + 3);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kOpenCurly);
        CHECK(tokens[3].start == code + 4);
        CHECK(tokens[3].length == 1);
        CHECK(tokens[4].type == Lexer::Token::Type::kCloseCurly);
        CHECK(tokens[4].start == code + 5);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[5].type == Lexer::Token::Type::kClassName);
        CHECK(tokens[5].start == code + 6);
        CHECK(tokens[5].length == 19);
        CHECK(tokens[6].type == Lexer::Token::Type::kColon);
        CHECK(tokens[6].start == code + 26);
        CHECK(tokens[6].length == 1);
        CHECK(tokens[7].type == Lexer::Token::Type::kClassName);
        CHECK(tokens[7].start == code + 28);
        CHECK(tokens[7].length == 14);
        CHECK(tokens[8].type == Lexer::Token::Type::kOpenCurly);
        CHECK(tokens[8].start == code + 43);
        CHECK(tokens[8].length == 1);
        CHECK(tokens[9].type == Lexer::Token::Type::kCloseCurly);
        CHECK(tokens[9].start == code + 45);
        CHECK(tokens[9].length == 1);
    }
    SUBCASE("extension") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "+Object{} + Numb3r { }";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 8);
        CHECK(tokens[0].type == Lexer::Token::Type::kPlus);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kClassName);
        CHECK(tokens[1].start == code + 1);
        CHECK(tokens[1].length == 6);
        CHECK(tokens[2].type == Lexer::Token::Type::kOpenCurly);
        CHECK(tokens[2].start == code + 7);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kCloseCurly);
        CHECK(tokens[3].start == code + 8);
        CHECK(tokens[3].length == 1);
        CHECK(tokens[4].type == Lexer::Token::Type::kPlus);
        CHECK(tokens[4].start == code + 10);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[5].type == Lexer::Token::Type::kClassName);
        CHECK(tokens[5].start == code + 12);
        CHECK(tokens[5].length == 6);
        CHECK(tokens[6].type == Lexer::Token::Type::kOpenCurly);
        CHECK(tokens[6].start == code + 19);
        CHECK(tokens[6].length == 1);
        CHECK(tokens[7].type == Lexer::Token::Type::kCloseCurly);
        CHECK(tokens[7].start == code + 21);
        CHECK(tokens[7].length == 1);
    }
    SUBCASE("method invocation") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "Class.method(label: 4)";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 8);
        CHECK(tokens[0].type == Lexer::Token::Type::kClassName);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 5);
        CHECK(tokens[1].type == Lexer::Token::Type::kDot);
        CHECK(tokens[1].start == code + 5);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[2].start == code + 6);
        CHECK(tokens[2].length == 6);
        CHECK(tokens[3].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[3].start == code + 12);
        CHECK(tokens[3].length == 1);
        CHECK(tokens[4].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[4].start == code + 13);
        CHECK(tokens[4].length == 5);
        CHECK(tokens[5].type == Lexer::Token::Type::kColon);
        CHECK(tokens[5].start == code + 18);
        CHECK(tokens[5].length == 1);
        CHECK(tokens[6].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[6].start == code + 20);
        CHECK(tokens[6].length == 1);
        CHECK(tokens[6].value.integer == 4);
        CHECK(tokens[7].type == Lexer::Token::Type::kCloseParen);
        CHECK(tokens[7].start == code + 21);
        CHECK(tokens[7].length == 1);
    }
    SUBCASE("construction") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "SynthDef(\\t, { SinOsc.ar(880) }).add;";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 16);
        CHECK(tokens[0].type == Lexer::Token::Type::kClassName);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 8);
        CHECK(tokens[1].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[1].start == code + 8);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kSymbol);
        CHECK(tokens[2].start == code + 10);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[3].type == Lexer::Token::Type::kComma);
        CHECK(tokens[3].start == code + 11);
        CHECK(tokens[3].length == 1);
        CHECK(tokens[4].type == Lexer::Token::Type::kOpenCurly);
        CHECK(tokens[4].start == code + 13);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[5].type == Lexer::Token::Type::kClassName);
        CHECK(tokens[5].start == code + 15);
        CHECK(tokens[5].length == 6);
        CHECK(tokens[6].type == Lexer::Token::Type::kDot);
        CHECK(tokens[6].start == code + 21);
        CHECK(tokens[6].length == 1);
        CHECK(tokens[7].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[7].start == code + 22);
        CHECK(tokens[7].length == 2);
        CHECK(tokens[8].type == Lexer::Token::Type::kOpenParen);
        CHECK(tokens[8].start == code + 24);
        CHECK(tokens[8].length == 1);
        CHECK(tokens[9].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[9].start == code + 25);
        CHECK(tokens[9].length == 3);
        CHECK(tokens[9].value.integer == 880);
        CHECK(tokens[10].type == Lexer::Token::Type::kCloseParen);
        CHECK(tokens[10].start == code + 28);
        CHECK(tokens[10].length == 1);
        CHECK(tokens[11].type == Lexer::Token::Type::kCloseCurly);
        CHECK(tokens[11].start == code + 30);
        CHECK(tokens[11].length == 1);
        CHECK(tokens[12].type == Lexer::Token::Type::kCloseParen);
        CHECK(tokens[12].start == code + 31);
        CHECK(tokens[12].length == 1);
        CHECK(tokens[13].type == Lexer::Token::Type::kDot);
        CHECK(tokens[13].start == code + 32);
        CHECK(tokens[13].length == 1);
        CHECK(tokens[14].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[14].start == code + 33);
        CHECK(tokens[14].length == 3);
        CHECK(tokens[15].type == Lexer::Token::Type::kSemicolon);
        CHECK(tokens[15].start == code + 36);
        CHECK(tokens[15].length == 1);
    }
}

TEST_CASE("Lexer Dots") {
    SUBCASE("valid dot patterns") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = ". .. ...";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0].type == Lexer::Token::Type::kDot);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kDotDot);
        CHECK(tokens[1].start == code + 2);
        CHECK(tokens[1].length == 2);
        CHECK(tokens[2].type == Lexer::Token::Type::kEllipses);
        CHECK(tokens[2].start == code + 5);
        CHECK(tokens[2].length == 3);
    }
    SUBCASE("invalid dot pattern") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "....";
        Lexer lexer(code);
        REQUIRE(!lex(lexer, tokens));
    }
    SUBCASE("method call") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "a.ham";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 1);
        CHECK(tokens[1].type == Lexer::Token::Type::kDot);
        CHECK(tokens[1].start == code + 1);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[2].start == code + 2);
        CHECK(tokens[2].length == 3);
    }
    SUBCASE("array slice") {
        std::vector<hadron::Lexer::Token> tokens;
        const char* code = "xR[9..0]";
        Lexer lexer(code);
        REQUIRE(lex(lexer, tokens));
        REQUIRE(tokens.size() == 6);
        CHECK(tokens[0].type == Lexer::Token::Type::kIdentifier);
        CHECK(tokens[0].start == code);
        CHECK(tokens[0].length == 2);
        CHECK(tokens[1].type == Lexer::Token::Type::kOpenSquare);
        CHECK(tokens[1].start == code + 2);
        CHECK(tokens[1].length == 1);
        CHECK(tokens[2].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[2].start == code + 3);
        CHECK(tokens[2].length == 1);
        CHECK(tokens[2].value.integer == 9);
        CHECK(tokens[3].type == Lexer::Token::Type::kDotDot);
        CHECK(tokens[3].start == code + 4);
        CHECK(tokens[3].length == 2);
        CHECK(tokens[4].type == Lexer::Token::Type::kInteger);
        CHECK(tokens[4].start == code + 6);
        CHECK(tokens[4].length == 1);
        CHECK(tokens[4].value.integer == 0);
        CHECK(tokens[5].type == Lexer::Token::Type::kCloseSquare);
        CHECK(tokens[5].start == code + 7);
        CHECK(tokens[5].length == 1);
    }
}

} // namespace hadron
