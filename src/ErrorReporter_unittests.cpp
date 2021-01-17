#include "ErrorReporter.hpp"

#include <doctest/doctest.h>

namespace hadron {

TEST_CASE("line numbers") {
    SUBCASE("empty string") {
        ErrorReporter er;
        std::string code("");
        er.setCode(code.data());
        CHECK(er.getLineNumber(code.data()) == 1);
    }
    SUBCASE("one liner") {
        ErrorReporter er;
        std::string code("I met a man with a wooden leg named Steve. Oh yeah? What was his other leg named?");
        er.setCode(code.data());
        CHECK(er.getLineNumber(code.data()) == 1);
        CHECK(er.getLineNumber(code.data() + 10) == 1);
        CHECK(er.getLineNumber(code.data() + code.size()) == 1);
    }
    SUBCASE("multiline string") {
        ErrorReporter er;
        std::string code("one\n two\n three\n four\n five\n six\n seven\n eight\n nine\n ten\n");
        er.setCode(code.data());
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
        ErrorReporter er;
        std::string code("\n\n\n\n\n\n\n7");
        er.setCode(code.data());
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
