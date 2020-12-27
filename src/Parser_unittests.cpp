#include "Parser.hpp"

#include "ErrorReporter.hpp"

#include "doctest/doctest.h"

namespace hadron {

TEST_CASE("Parser Base Case") {
    SUBCASE("empty string") {
        Parser parser("", std::make_shared<ErrorReporter>());
        REQUIRE(parser.parse());
        REQUIRE(parser.root() != nullptr);
        CHECK(parser.root()->nodeType == parse::NodeType::kEmpty);
        CHECK(parser.root()->tokenIndex == 0);
        CHECK(parser.root()->next.get() == nullptr);
        CHECK(parser.root()->tail == parser.root());
    }
}

} // namespace hadron
