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
        Assembler assembler("alias %vr2\n"
                            "alias %vr1\n"
                            "alias %vr0\n"
                            "addr %vr2 %vr0 %vr1");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 4);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 2});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 1});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kAlias, 0});
        CHECK(assembler.virtualJIT()->instructions()[3] == VirtualJIT::Inst{VirtualJIT::kAddr, 2, 0, 1});
    }
    SUBCASE("addi") {
        Assembler assembler("alias %vr4\n"
                            "alias %vr10\n"
                            "addi %vr4 %vr10 -128");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 4});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 10});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kAddi, 4, 10, -128});
    }
    SUBCASE("movr") {
        Assembler assembler("alias %vr0\n"
                            "alias %vr1\n"
                            "movr %vr0 %vr1\n");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 0});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 1});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kMovr, 0, 1});
    }
    SUBCASE("movi") {
        Assembler assembler("alias %vr0\n"
                            "movi %vr0 24");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 2);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 0});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kMovi, 0, 24});
    }
    SUBCASE("bgei") {
        Assembler assembler("alias %vr7\n"
                            "bgei %vr7 42 label_7");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 2);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 7});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kBgei, 7, 42});
    }
    SUBCASE("jmp") {
        Assembler assembler("jmp");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kJmp, 0});
    }
    SUBCASE("jmpr") {
        Assembler assembler("jmpr %vr2");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kJmpR, 2});
    }
    SUBCASE("ldxi_w") {
        Assembler assembler("alias %vr2\n"
                            "alias %vr1\n"
                            "ldxi_w %vr2 %vr1 0xaf");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 2});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 1});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kLdxiW, 2, 1, 0xaf});
    }
    SUBCASE("ldxi_i") {
        Assembler assembler("alias %vr2\n"
                            "alias %vr1\n"
                            "ldxi_i %vr2 %vr1 0xaf");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 2});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 1});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kLdxiI, 2, 1, 0xaf});
    }
    SUBCASE("ldxi_l") {
        Assembler assembler("alias %vr2\n"
                            "alias %vr1\n"
                            "ldxi_l %vr2 %vr1 0xaf");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 2});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 1});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kLdxiL, 2, 1, 0xaf});
    }
    SUBCASE("str_i") {
        Assembler assembler("alias %vr4\n"
                            "alias %vr0\n"
                            "str_i %vr4 %vr0");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 4});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 0});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kStrI, 4, 0});
    }
    SUBCASE("stxi_w") {
        Assembler assembler("alias %vr1\n"
                            "alias %vr10\n"
                            "stxi_w 0x4 %vr1 %vr10");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 1});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 10});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kStxiW, 4, 1, 10});
    }
    SUBCASE("stxi_i") {
        Assembler assembler("alias %vr1\n"
                            "alias %vr10\n"
                            "stxi_i 0x4 %vr1 %vr10");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 1});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 10});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kStxiI, 4, 1, 10});
    }
    SUBCASE("stxi_l") {
        Assembler assembler("alias %vr1\n"
                            "alias %vr10\n"
                            "stxi_l 0x4 %vr1 %vr10");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 1});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 10});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kStxiL, 4, 1, 10});
    }
    SUBCASE("ret") {
        Assembler assembler("ret");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kRet});
    }
    SUBCASE("retr") {
        Assembler assembler("alias %vr9\n"
                            "retr %vr9");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 2);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 9});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kRetr, 9});
    }
    SUBCASE("reti") {
        Assembler assembler("reti 99");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kReti, 99});
    }
    SUBCASE("label") {
        Assembler assembler("label");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kLabel});
    }
    SUBCASE("patch_here") {
        Assembler assembler("label\n"
                            "patch_here label_0");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 2);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kLabel});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kPatchHere, 0});
    }
    SUBCASE("patch_there") {
        Assembler assembler("label\n"
                            "label\n"
                            "patch_there label_0 label_1");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kLabel});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kLabel});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kPatchThere, 0, 1});
    }
    SUBCASE("alias") {
        Assembler assembler("alias %vr0");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 0});
    }
    SUBCASE("unalias") {
        Assembler assembler("unalias %vr7");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kUnalias, 7});
    }
}

} // namespace hadron