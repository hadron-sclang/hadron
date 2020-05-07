#include "Lexer.hpp"

#include "doctest/doctest.h"

#include <vector>

namespace hadron {

TEST_CASE("Lexer base cases") {
    std::vector<Lexer::Token> tokens;
    SUBCASE("empty string") {
        CHECK(Lexer::Lex("", tokens) == true);
    }
}

} // namespace hadron