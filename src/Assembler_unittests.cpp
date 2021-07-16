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
    SUBCASE("jmpi") {
        Assembler assembler("jmpi label_22");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kJmpi, 0});
    }
    SUBCASE("ldxi") {
        Assembler assembler("alias %vr2\n"
                            "alias %vr1\n"
                            "ldxi %vr2 %vr1 0xaf");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 2});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 1});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kLdxi, 2, 1, 0xaf});
    }
    SUBCASE("str") {
        Assembler assembler("alias %vr4\n"
                            "alias %vr0\n"
                            "str %vr4 %vr0");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 4});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 0});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kStr, 4, 0});
    }
    SUBCASE("sti") {
        Assembler assembler("alias %vr0\n"
                            "sti 0x25 %vr0");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 2);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 0});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kSti, 0, 0});
        REQUIRE(assembler.virtualJIT()->addresses().size() == 1);
        CHECK(assembler.virtualJIT()->addresses()[0] == reinterpret_cast<void*>(0x25));
    }
    SUBCASE("stxi") {
        Assembler assembler("alias %vr1\n"
                            "alias %vr10\n"
                            "stxi 0x4 %vr1 %vr10");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 1});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kAlias, 10});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kStxi, 4, 1, 10});
    }
    SUBCASE("prolog") {
        Assembler assembler("prolog");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kProlog});
    }
    SUBCASE("arg") {
        Assembler assembler("arg");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kArg, 0});
    }
    SUBCASE("getarg") {
        Assembler assembler("alias %vr0\n"
                            "getarg %vr0 label_0");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 2);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAlias, 0});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kGetarg, 0, 0});
    }
    SUBCASE("allocai") {
        Assembler assembler("allocai 1024");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kAllocai, 1024});
    }
    SUBCASE("allocai") {
        Assembler assembler("frame 0");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kFrame, 0});
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
    SUBCASE("epilog") {
        Assembler assembler("epilog");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kEpilog});
    }
    SUBCASE("label") {
        Assembler assembler("label");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 1);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kLabel});
    }
    SUBCASE("patchat") {
        Assembler assembler("label\n"
                            "label\n"
                            "patchat label_0 label_1");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 3);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kLabel});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kLabel});
        CHECK(assembler.virtualJIT()->instructions()[2] == VirtualJIT::Inst{VirtualJIT::kPatchAt, 0, 1});
    }
    SUBCASE("patch") {
        Assembler assembler("label\n"
                            "patch label_0");
        REQUIRE(assembler.assemble());
        REQUIRE(assembler.virtualJIT()->instructions().size() == 2);
        CHECK(assembler.virtualJIT()->instructions()[0] == VirtualJIT::Inst{VirtualJIT::kLabel});
        CHECK(assembler.virtualJIT()->instructions()[1] == VirtualJIT::Inst{VirtualJIT::kPatch, 0});
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