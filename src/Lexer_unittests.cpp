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
    SUBCASE("zero elided <DIFFA1>") {
        const char* code = "0x";
        Lexer lexer(code);
        REQUIRE(!lexer.lex());
        REQUIRE(lexer.tokens().size() == 0);
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
    SUBCASE("multi-digit mikxed") {
        const char* code = "0x1A2b3C";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 8);
        CHECK(lexer.tokens()[0].value.integer == 0x1a2b3c);
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

TEST_CASE("Strings") {
    SUBCASE("simple string") {
        const char* code = "\"abc\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kString);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 5);
    }
    SUBCASE("padded string") {
        const char* code = "  \"Spaces inside and out.\"  ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kString);
        CHECK(lexer.tokens()[0].start == code + 2);
        CHECK(lexer.tokens()[0].length == 24);
    }
    SUBCASE("special characters") {
        const char* code = "\"\t\n\r\\t\\r\\n\\\"0x'\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kString);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 16);
    }
    SUBCASE("adjacent strings tight") {
        const char* code = "\"a\"\"b\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kString);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kString);
        CHECK(lexer.tokens()[1].start == code + 3);
        CHECK(lexer.tokens()[1].length == 3);
    }
    SUBCASE("adjacent strings padded") {
        const char* code = "  \"\\\"\"  \"b\"  ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 2);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kString);
        CHECK(lexer.tokens()[0].start == code + 2);
        CHECK(lexer.tokens()[0].length == 4);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kString);
        CHECK(lexer.tokens()[1].start == code + 8);
        CHECK(lexer.tokens()[1].length == 3);
    }
    SUBCASE("unterminated string") {
        Lexer lexer("\"abc");
        REQUIRE(!lexer.lex());
    }
}

TEST_CASE("Symbols") {
    SUBCASE("empty quote symbol") {
        const char* code = "''";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kSymbol);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 2);
    }
    SUBCASE("simple quote symbol") {
        const char* code = "'bA1'";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kSymbol);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 5);
    }
    SUBCASE("padded quote symbol") {
        const char* code = "  'ALL CAPS READS LIKE SHOUTING'  ";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kSymbol);
        CHECK(lexer.tokens()[0].start == code + 2);
        CHECK(lexer.tokens()[0].length == 30);
    }
    SUBCASE("special characters") {
        const char* code = "'\\t\\n\\r\t\n\r\\'0x\"'";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kSymbol);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 16);
    }
    SUBCASE("unterminated quote symbol") {
        Lexer lexer("'abc");
        REQUIRE(!lexer.lex());
    }
}

TEST_CASE("Addition") {
    SUBCASE("bare plus") {
        const char* code = "+";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kAddition);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
    }
    SUBCASE("two integers padded") {
        const char* code = "1 + 22";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.integer == 1);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[1].start) == '+');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[2].start == code + 4);
        CHECK(lexer.tokens()[2].length == 2);
        CHECK(lexer.tokens()[2].value.integer == 22);
    }
    SUBCASE("two integers tight") {
        const char* code = "67+4";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 2);
        CHECK(lexer.tokens()[0].value.integer == 67);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[1].start) == '+');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[2].start == code + 3);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.integer == 4);
    }
    SUBCASE("tight left") {
        const char* code = "7+ 0x17";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.integer == 7);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[1].start) == '+');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[2].start == code + 3);
        CHECK(lexer.tokens()[2].length == 4);
        CHECK(lexer.tokens()[2].value.integer == 0x17);
    }
    SUBCASE("tight right") {
        const char* code = "0xffe +93";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 5);
        CHECK(lexer.tokens()[0].value.integer == 0xffe);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[1].start) == '+');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[2].start == code + 7);
        CHECK(lexer.tokens()[2].length == 2);
        CHECK(lexer.tokens()[2].value.integer == 93);
    }
    SUBCASE("zeros tight") {
        const char* code = "0+0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.integer == 0);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[1].start) == '+');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[2].start == code + 2);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.integer == 0);
    }
    SUBCASE("zeros padded") {
        const char* code = "0 + 0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.integer == 0);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[1].start) == '+');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[2].start == code + 4);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.integer == 0);
    }
    SUBCASE("zeros tight left") {
        const char* code = "0+ 0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.integer == 0);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[1].start) == '+');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[2].start == code + 3);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.integer == 0);
    }
    SUBCASE("zeros tight right") {
        const char* code = "0 +0";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.integer == 0);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[1].start) == '+');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[2].start == code + 3);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.integer == 0);
    }
    SUBCASE("chaining integers") {
        const char* code = "0+1+2 + 0x3+ 4 +5";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 11);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 1);
        CHECK(lexer.tokens()[0].value.integer == 0);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[1].start) == '+');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[2].start == code + 2);
        CHECK(lexer.tokens()[2].length == 1);
        CHECK(lexer.tokens()[2].value.integer == 1);
        CHECK(lexer.tokens()[3].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[3].start) == '+');
        CHECK(lexer.tokens()[3].length == 1);
        CHECK(lexer.tokens()[4].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[4].start == code + 4);
        CHECK(lexer.tokens()[4].length == 1);
        CHECK(lexer.tokens()[4].value.integer == 2);
        CHECK(lexer.tokens()[5].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[5].start) == '+');
        CHECK(lexer.tokens()[5].length == 1);
        CHECK(lexer.tokens()[6].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[6].start == code + 8);
        CHECK(lexer.tokens()[6].length == 3);
        CHECK(lexer.tokens()[6].value.integer == 3);
        CHECK(lexer.tokens()[7].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[7].start) == '+');
        CHECK(lexer.tokens()[7].length == 1);
        CHECK(lexer.tokens()[8].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[8].start == code + 13);
        CHECK(lexer.tokens()[8].length == 1);
        CHECK(lexer.tokens()[8].value.integer == 4);
        CHECK(lexer.tokens()[9].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[9].start) == '+');
        CHECK(lexer.tokens()[9].length == 1);
        CHECK(lexer.tokens()[10].type == Lexer::Token::Type::kInteger);
        CHECK(lexer.tokens()[10].start == code + 16);
        CHECK(lexer.tokens()[10].length == 1);
        CHECK(lexer.tokens()[10].value.integer == 5);
    }
    SUBCASE("strings tight") {
        const char* code = "\"a\"+\"bcdefg\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kString);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 3);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[1].start) == '+');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kString);
        CHECK(lexer.tokens()[2].start == code + 4);
        CHECK(lexer.tokens()[2].length == 8);
    }
    SUBCASE("strings padded") {
        const char* code = "\"0123\" + \"ABCD\"";
        Lexer lexer(code);
        REQUIRE(lexer.lex());
        REQUIRE(lexer.tokens().size() == 3);
        CHECK(lexer.tokens()[0].type == Lexer::Token::Type::kString);
        CHECK(lexer.tokens()[0].start == code);
        CHECK(lexer.tokens()[0].length == 6);
        CHECK(lexer.tokens()[1].type == Lexer::Token::Type::kAddition);
        CHECK(*(lexer.tokens()[1].start) == '+');
        CHECK(lexer.tokens()[1].length == 1);
        CHECK(lexer.tokens()[2].type == Lexer::Token::Type::kString);
        CHECK(lexer.tokens()[2].start == code + 9);
        CHECK(lexer.tokens()[2].length == 6);
    }
}

} // namespace hadron
