#include "hadron/Assembler.hpp"

#include "hadron/VirtualJIT.hpp"

#include "doctest/doctest.h"

namespace hadron {

TEST_CASE("Assembler Base Cases") {
    SUBCASE("empty string") {
        Assembler assembler("");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 0);
    }
    SUBCASE("addr") {
        Assembler assembler("addr %vr2 %vr0 %vr1");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAddr, 2, 0, 1});
    }
    SUBCASE("addi") {
        Assembler assembler("addi %vr4 %vr10 -128");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAddi, 4, 10, -128});
    }
    SUBCASE("movr") {
        Assembler assembler("movr %vr0 %vr1");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kMovr, 0, 1});
    }
    SUBCASE("movi") {
        Assembler assembler("movi %vr0 24");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kMovi, 0, 24});
    }
    SUBCASE("bgei") {
        Assembler assembler("bgei %vr7 42 label_7");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kBgei, 7, 42});
    }
    SUBCASE("jmpi") {
        Assembler assembler("jmpi label_22");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kJmpi, 0});
    }
    SUBCASE("ldxi") {
        Assembler assembler("ldxi %vr2 %vr1 0xaf");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kLdxi, 2, 1, 0xaf});
    }
    SUBCASE("str") {
        Assembler assembler("str %vr4 %vr0");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kStr, 4, 0});
    }
}

} // namespace hadron