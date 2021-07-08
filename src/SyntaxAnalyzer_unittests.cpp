#include "hadron/SyntaxAnalyzer.hpp"

#include "hadron/ErrorReporter.hpp"

#include "doctest/doctest.h"

namespace hadron {

TEST_CASE("Identifier Resolution") {
    SUBCASE("local variable") {
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

}

TEST_CASE("Binary Operator Lowering") {

}

TEST_CASE("Control Flow Lowering") {
}

TEST_CASE("Implied Return Assignment") {
}

TEST_CASE("Inlining Literal Blocks") {
    SUBCASE("variable scope hoisting") {
    }
}

TEST_CASE("Class Construction") {
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