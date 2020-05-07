#include "Lexer.hpp"

#include "doctest/doctest.h"

#include <vector>

namespace hadron {

TEST_CASE("Lexer Base Cases") {
    SUBCASE("empty string") {
        Lexer lexer("");
        CHECK(lexer.lex());
        CHECK(lexer.tokens().size() == 0);
    }
}

TEST_CASE("Lexer Integers") {
    SUBCASE("simple integers") {
        Lexer lexer("0");
        CHECK(lexer.lex());
        CHECK(lexer.tokens().size() == 1);
        CHECK(lexer.tokens()[0].start == 0);
        CHECK(lexer.tokens()[0].length == 1);
    }
}

} // namespace hadron
