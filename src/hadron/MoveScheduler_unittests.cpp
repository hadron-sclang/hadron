#include "hadron/MoveScheduler.hpp"

#include "hadron/library/Dictionary.hpp"
#include "hadron/library/LibraryTestFixture.hpp"
#include "hadron/OpcodeIterator.hpp"
#include "hadron/Slot.hpp"
#include "hadron/VirtualJIT.hpp"

#include "doctest/doctest.h"
#include "spdlog/spdlog.h"

#include <array>
#include <unordered_map>

namespace hadron {

TEST_CASE_FIXTURE(LibraryTestFixture, "MoveScheduler") {
    SUBCASE("empty set") {
        VirtualJIT jit;
        auto moves = library::TypedIdentDict<library::Integer, library::Integer>::makeTypedIdentDict(context());
        MoveScheduler ms;
        std::array<int8_t, 16> jitBuffer;
        jit.begin(jitBuffer.data(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);
        CHECK_EQ(finalSize, 0);
    }

    SUBCASE("register to register") {
        VirtualJIT jit;
        auto moves = library::TypedIdentDict<library::Integer, library::Integer>::makeTypedIdentDict(context());
        moves.typedPut(context(), library::Integer(3), library::Integer(2));
        MoveScheduler ms;
        std::array<int8_t, 16> jitBuffer;
        jit.begin(jitBuffer.data(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);
        REQUIRE_EQ(finalSize, 3);

        OpcodeReadIterator it(jitBuffer.data(), jitBuffer.size());
        REQUIRE_EQ(it.peek(), Opcode::kMovr);
        JIT::Reg target, value;
        REQUIRE(it.movr(target, value));
        CHECK_EQ(target, 2);
        CHECK_EQ(value, 3);
    }

    SUBCASE("register to spill") {
        VirtualJIT jit;
        auto moves = library::TypedIdentDict<library::Integer, library::Integer>::makeTypedIdentDict(context());
        moves.typedPut(context(), library::Integer(0), library::Integer(-1));
        MoveScheduler ms;
        std::array<int8_t, 16> jitBuffer;
        jit.begin(jitBuffer.data(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);
        REQUIRE_EQ(finalSize, 7);

        OpcodeReadIterator it(jitBuffer.data(), jitBuffer.size());
        REQUIRE_EQ(it.peek(), Opcode::kStxiL);
        int offset;
        JIT::Reg address, value;
        REQUIRE(it.stxi_l(offset, address, value));
        CHECK_EQ(offset, -1 * kSlotSize);
        CHECK_EQ(address, JIT::kStackPointerReg);
        CHECK_EQ(value, 0);
    }

    SUBCASE("spill to register") {
        VirtualJIT jit;
        auto moves = library::TypedIdentDict<library::Integer, library::Integer>::makeTypedIdentDict(context());
        moves.typedPut(context(), library::Integer(-24), library::Integer(5));
        MoveScheduler ms;
        std::array<int8_t, 16> jitBuffer;
        jit.begin(jitBuffer.data(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);
        REQUIRE_EQ(finalSize, 7);

        OpcodeReadIterator it(jitBuffer.data(), jitBuffer.size());
        REQUIRE_EQ(it.peek(), Opcode::kLdxiL);
        JIT::Reg target, address;
        int offset;
        REQUIRE(it.ldxi_l(target, address, offset));
        CHECK_EQ(target, 5);
        CHECK_EQ(address, JIT::kStackPointerReg);
        CHECK_EQ(offset, -24 * kSlotSize);
    }

    SUBCASE("multiple independent moves") {
        VirtualJIT jit;
        auto moves = library::TypedIdentDict<library::Integer, library::Integer>::makeTypedIdentDict(context());
        moves.typedPut(context(), library::Integer(-3), library::Integer( 2));
        moves.typedPut(context(), library::Integer( 9), library::Integer( 7));
        moves.typedPut(context(), library::Integer( 3), library::Integer(-1));
        MoveScheduler ms;
        std::array<int8_t, 32> jitBuffer;
        jit.begin(jitBuffer.data(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);
        OpcodeReadIterator it(jitBuffer.begin(), jitBuffer.size());
        bool gotMinusThree = false;
        bool gotNine = false;
        bool gotThree = false;

        while (it.getSize() < finalSize) {
            if (it.peek() == Opcode::kLdxiL) {
                JIT::Reg target, address;
                int offset;
                REQUIRE(it.ldxi_l(target, address, offset));
                CHECK_EQ(target, 2);
                CHECK_EQ(address, JIT::kStackPointerReg);
                CHECK_EQ(offset, -3 * kSlotSize);
                CHECK(!gotMinusThree);
                gotMinusThree = true;
            } else if (it.peek() == Opcode::kMovr) {
                JIT::Reg target, value;
                REQUIRE(it.movr(target, value));
                CHECK_EQ(target, 7);
                CHECK_EQ(value, 9);
                CHECK(!gotNine);
                gotNine = true;
            } else if (it.peek() == Opcode::kStxiL) {
                int offset;
                JIT::Reg address, value;
                REQUIRE(it.stxi_l(offset, address, value));
                CHECK_EQ(offset, -1 * kSlotSize);
                CHECK_EQ(address, JIT::kStackPointerReg);
                CHECK_EQ(value, 3);
                CHECK(!gotThree);
                gotThree = true;
            }
        }
        CHECK_EQ(it.getSize(), finalSize);
        CHECK(gotMinusThree);
        CHECK(gotNine);
        CHECK(gotThree);
    }

    SUBCASE("Two Chain") {
        VirtualJIT jit;
        auto moves = library::TypedIdentDict<library::Integer, library::Integer>::makeTypedIdentDict(context());
        moves.typedPut(context(), library::Integer(3), library::Integer(2));
        moves.typedPut(context(), library::Integer(2), library::Integer(1));
        MoveScheduler ms;
        std::array<int8_t, 16> jitBuffer;
        jit.begin(jitBuffer.data(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);

        std::array<int8_t, 16> desiredBuffer;
        OpcodeWriteIterator it(desiredBuffer.data(), desiredBuffer.size());
        // The 1 <- 2 move needs to happen before the 2 <- 3 move.
        it.movr(1, 2);
        it.movr(2, 3);
        REQUIRE_EQ(finalSize, it.getSize());
        CHECK_EQ(std::memcmp(jitBuffer.data(), desiredBuffer.data(), finalSize), 0);
    }

    SUBCASE("Three Chain Through Spill") {
        VirtualJIT jit;
        auto moves = library::TypedIdentDict<library::Integer, library::Integer>::makeTypedIdentDict(context());
        moves.typedPut(context(), library::Integer( 0), library::Integer(3));
        moves.typedPut(context(), library::Integer( 3), library::Integer(1));
        moves.typedPut(context(), library::Integer(-1), library::Integer(0));
        MoveScheduler ms;
        std::array<int8_t, 32> jitBuffer;
        jit.begin(jitBuffer.begin(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);

        std::array<int8_t, 32> desiredBuffer;
        OpcodeWriteIterator it(desiredBuffer.data(), desiredBuffer.size());
        // Order needs to be 1 <- 3, 3 <- 0, 0 <- -1
        it.movr(1, 3);
        it.movr(3, 0);
        it.ldxi_l(0, JIT::kStackPointerReg, -1 * kSlotSize);
        REQUIRE_EQ(finalSize, it.getSize());
        CHECK_EQ(std::memcmp(jitBuffer.data(), desiredBuffer.data(), finalSize), 0);
    }

    SUBCASE("Eight Chain Unordered") {
        VirtualJIT jit;
        auto moves = library::TypedIdentDict<library::Integer, library::Integer>::makeTypedIdentDict(context());
        moves.typedPut(context(), library::Integer(6), library::Integer(5));
        moves.typedPut(context(), library::Integer(2), library::Integer(1));
        moves.typedPut(context(), library::Integer(5), library::Integer(4));
        moves.typedPut(context(), library::Integer(7), library::Integer(6));
        moves.typedPut(context(), library::Integer(4), library::Integer(3));
        moves.typedPut(context(), library::Integer(1), library::Integer(0));
        moves.typedPut(context(), library::Integer(3), library::Integer(2));
        moves.typedPut(context(), library::Integer(8), library::Integer(7));
        MoveScheduler ms;
        std::array<int8_t, 64> jitBuffer;
        jit.begin(jitBuffer.data(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);

        std::array<int8_t, 64> desiredBuffer;
        OpcodeWriteIterator it(desiredBuffer.data(), desiredBuffer.size());
        it.movr(0, 1);
        it.movr(1, 2);
        it.movr(2, 3);
        it.movr(3, 4);
        it.movr(4, 5);
        it.movr(5, 6);
        it.movr(6, 7);
        it.movr(7, 8);
        REQUIRE_EQ(finalSize, it.getSize());
        CHECK_EQ(std::memcmp(jitBuffer.data(), desiredBuffer.data(), finalSize), 0);
    }

    SUBCASE("Two Simple Cycles") {
        VirtualJIT jit;
        auto moves = library::TypedIdentDict<library::Integer, library::Integer>::makeTypedIdentDict(context());
        moves.typedPut(context(), library::Integer(0), library::Integer(3));
        moves.typedPut(context(), library::Integer(2), library::Integer(1));
        moves.typedPut(context(), library::Integer(1), library::Integer(2));
        moves.typedPut(context(), library::Integer(3), library::Integer(0));

        MoveScheduler ms;
        std::array<int8_t, 32> jitBuffer;
        jit.begin(jitBuffer.data(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);

        std::array<bool, 4> regSet{false, false, false, false};

        // Instructions should be all xor operations and every target register should be set at least once.
        OpcodeReadIterator it(jitBuffer.data(), jitBuffer.size());
        while (it.getSize() < finalSize) {
            REQUIRE(it.peek() == Opcode::kXorr);
            JIT::Reg target, a, b;
            REQUIRE(it.xorr(target, a, b));
            regSet[target] = true;
        }

        for (auto bit : regSet) {
            CHECK(bit);
        }
    }

    SUBCASE("Three Cycle") {
        VirtualJIT jit;
        auto moves = library::TypedIdentDict<library::Integer, library::Integer>::makeTypedIdentDict(context());
        moves.typedPut(context(), library::Integer(0), library::Integer(2));
        moves.typedPut(context(), library::Integer(1), library::Integer(0));
        moves.typedPut(context(), library::Integer(2), library::Integer(1));
        MoveScheduler ms;
        std::array<int8_t, 32> jitBuffer;
        jit.begin(jitBuffer.data(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);

        OpcodeReadIterator it(jitBuffer.data(), jitBuffer.size());
        // First operation should be a store to the temporary slot.
        REQUIRE(it.peek() == Opcode::kStrL);
        JIT::Reg address, chainReg;
        REQUIRE(it.str_l(address, chainReg));
        CHECK_EQ(address, JIT::kStackPointerReg);
        // Second two operations are moves within the cycle.
        REQUIRE_EQ(it.peek(), Opcode::kMovr);
        JIT::Reg target, value;
        REQUIRE(it.movr(target, value));
        CHECK_EQ(target, chainReg);
        CHECK_EQ(value, (chainReg + 1) % 3);
        REQUIRE_EQ(it.peek(), Opcode::kMovr);
        REQUIRE(it.movr(target, value));
        CHECK_EQ(target, (chainReg + 1) % 3);
        CHECK_EQ(value, (chainReg + 2) % 3);
        // Last instruction is a load into the last reg in the chain.
        REQUIRE_EQ(it.peek(), Opcode::kLdrL);
        REQUIRE(it.ldr_l(target, address));
        CHECK_EQ(target, (chainReg + 2) % 3);
        CHECK_EQ(address, JIT::kStackPointerReg);
    }
}

} // namespace hadron