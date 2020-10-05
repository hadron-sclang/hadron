#include "ErrorReporter.hpp"

#include <doctest/doctest.h>

namespace hadron {

TEST_CASE("line numbers") {
    SUBCASE("empty string") {
        std::string code("");
        ErrorReporter er(code.data());
        CHECK(er.getLineNumber(code.data()) == 1);
    }
    SUBCASE("one liner") {
        std::string code("I met a man with a wooden leg named Steve. Oh yeah? What was his other leg named?");
        ErrorReporter er(code.data());
        CHECK(er.getLineNumber(code.data()) == 1);
        CHECK(er.getLineNumber(code.data() + 10) == 1);
        CHECK(er.getLineNumber(code.data() + code.size()) == 1);
    }
    SUBCASE("multiline string") {
        std::string code("one\n two\n three\n four\n five\n six\n seven\n eight\n nine\n ten\n");
        ErrorReporter er(code.data());
        CHECK(er.getLineNumber(code.data() + 1) == 1);
        CHECK(er.getLineNumber(code.data() + 4) == 2);
        CHECK(er.getLineNumber(code.data() + 9) == 3);
        CHECK(er.getLineNumber(code.data() + 16) == 4);
        CHECK(er.getLineNumber(code.data() + 22) == 5);
        CHECK(er.getLineNumber(code.data() + 28) == 6);
        CHECK(er.getLineNumber(code.data() + 33) == 7);
        CHECK(er.getLineNumber(code.data() + 40) == 8);
        CHECK(er.getLineNumber(code.data() + 47) == 9);
        CHECK(er.getLineNumber(code.data() + 53) == 10);
    }
    SUBCASE("multiple empty lines") {
        std::string code("\n\n\n\n\n\n\n7");
        ErrorReporter er(code.data());
        CHECK(er.getLineNumber(code.data()) == 1);
        CHECK(er.getLineNumber(code.data() + 1) == 2);
        CHECK(er.getLineNumber(code.data() + 2) == 3);
        CHECK(er.getLineNumber(code.data() + 3) == 4);
        CHECK(er.getLineNumber(code.data() + 4) == 5);
        CHECK(er.getLineNumber(code.data() + 5) == 6);
        CHECK(er.getLineNumber(code.data() + 6) == 7);
    }
}

} // namespace hadron
