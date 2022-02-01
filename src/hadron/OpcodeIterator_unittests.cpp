#include "hadron/OpcodeIterator.hpp"

#include "hadron/Arch.hpp"
#include "hadron/JIT.hpp"

#include "doctest/doctest.h"

#include <array>

namespace hadron {

TEST_CASE("MoveIterator types") {
    SUBCASE("registers de/serialization") {
        std::array<int8_t, 16> buffer;
        OpcodeWriteIterator wIt(buffer.data(), buffer.size());
        REQUIRE(wIt.addr(JIT::kContextPointerReg, JIT::kStackPointerReg, 27));
        OpcodeReadIterator rIt(buffer.data(), buffer.size());
        JIT::Reg target, a, b;
        REQUIRE(rIt.addr(target, a, b));
        CHECK_EQ(target, JIT::kContextPointerReg);
        CHECK_EQ(a, JIT::kStackPointerReg);
        CHECK_EQ(b, 27);
    }

    SUBCASE("words de/serialization") {
        std::array<int8_t, 32> buffer;
        OpcodeWriteIterator wIt(buffer.data(), buffer.size());
        REQUIRE(wIt.addi(0, 1, 512));
        REQUIRE(wIt.addi(2, 3, -768));
        OpcodeReadIterator rIt(buffer.data(), buffer.size());
        JIT::Reg target, a;
        Word b;
        REQUIRE(rIt.addi(target, a, b));
        CHECK_EQ(target, 0);
        CHECK_EQ(a, 1);
        CHECK_EQ(b, 512);
        REQUIRE(rIt.addi(target, a, b));
        CHECK_EQ(target, 2);
        CHECK_EQ(a, 3);
        CHECK_EQ(b, -768);
    }

    SUBCASE("uwords de/serialization") {
        std::array<int8_t, 16> buffer;
        OpcodeWriteIterator wIt(buffer.data(), buffer.size());
        REQUIRE(wIt.andi(15, 30, 0xbad1dea));
        OpcodeReadIterator rIt(buffer.data(), buffer.size());
        JIT::Reg target, a;
        UWord b;
        REQUIRE(rIt.andi(target, a, b));
        CHECK_EQ(target, 15);
        CHECK_EQ(a, 30);
        CHECK_EQ(b, 0xbad1dea);
    }

    SUBCASE("integers de/serialization") {
        std::array<int8_t, 32> buffer;
        OpcodeWriteIterator wIt(buffer.data(), buffer.size());
        REQUIRE(wIt.ldxi_w(JIT::kStackPointerReg, 19, -16));
        REQUIRE(wIt.ldxi_w(4, JIT::kContextPointerReg, 4));
        OpcodeReadIterator rIt(buffer.data(), buffer.size());
        JIT::Reg target, address;
        int offset;
        REQUIRE(rIt.ldxi_w(target, address, offset));
        CHECK_EQ(target, JIT::kStackPointerReg);
        CHECK_EQ(address, 19);
        CHECK_EQ(offset, -16);
        REQUIRE(rIt.ldxi_w(target, address, offset));
        CHECK_EQ(target, 4);
        CHECK_EQ(address, JIT::kContextPointerReg);
        CHECK_EQ(offset, 4);
    }
}


} // namespace hadron