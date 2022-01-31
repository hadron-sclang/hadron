#include "hadron/MoveScheduler.hpp"

#include "hadron/OpcodeIterator.hpp"
#include "hadron/Slot.hpp"
#include "hadron/VirtualJIT.hpp"

#include "doctest/doctest.h"
#include "spdlog/spdlog.h"

#include <array>
#include <unordered_map>

namespace hadron {

TEST_CASE("MoveScheduler simple") {
    SUBCASE("empty set") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves;
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
        std::unordered_map<int, int> moves({{3, 2}});
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
        std::unordered_map<int, int> moves({{0, -1}});
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
        std::unordered_map<int, int> moves({{-24, 5}});
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
        std::unordered_map<int, int> moves({{-3, 2}, {9, 7}, {3, -1}});
        MoveScheduler ms;
        std::array<int8_t, 32> jitBuffer;
        jit.begin(jitBuffer.data(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);
        OpcodeReadIterator it(jitBuffer.begin(), jitBuffer.size());
        // Instructions can be in any order so check all three and remove from the map as we encounter.
        while (it.getSize() < finalSize) {
            std::unordered_map<int, int>::iterator mapIter = moves.end();
            if (it.peek() == Opcode::kLdxiL) {
                JIT::Reg target, address;
                int offset;
                REQUIRE(it.ldxi_l(target, address, offset));
                CHECK_EQ(target, 2);
                CHECK_EQ(address, JIT::kStackPointerReg);
                CHECK_EQ(offset, -3 * kSlotSize);
                mapIter = moves.find(-3);
            } else if (it.peek() == Opcode::kMovr) {
                JIT::Reg target, value;
                REQUIRE(it.movr(target, value));
                CHECK_EQ(target, 7);
                CHECK_EQ(value, 9);
                mapIter = moves.find(9);
            } else if (it.peek() == Opcode::kStxiL) {
                int offset;
                JIT::Reg address, value;
                REQUIRE(it.stxi_l(offset, address, value));
                CHECK_EQ(offset, -1 * kSlotSize);
                CHECK_EQ(address, JIT::kStackPointerReg);
                CHECK_EQ(value, 3);
                mapIter = moves.find(3);
            }
            REQUIRE(mapIter != moves.end());
            moves.erase(mapIter);
        }
        CHECK_EQ(it.getSize(), finalSize);
        CHECK(moves.empty());
    }
}

TEST_CASE("MoveScheduler Chains") {
    SUBCASE("Two Chain") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{3, 2}, {2, 1}});
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
        std::unordered_map<int, int> moves({{0, 3}, {3, 1}, {-1, 0}});
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
        std::unordered_map<int, int> moves({{6, 5}, {2, 1}, {5, 4}, {7, 6}, {4, 3}, {1, 0}, {3, 2}, {8, 7}});
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
}

TEST_CASE("MoveScheduler Cycles") {
    SUBCASE("Two Simple Cycles") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{0, 3}, {2, 4}, {4, 2}, {3, 0}});
        MoveScheduler ms;
        std::array<int8_t, 32> jitBuffer;
        jit.begin(jitBuffer.data(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);

        // Instructions should be all xor operations and every target register should be set at least once.
        OpcodeReadIterator it(jitBuffer.data(), jitBuffer.size());
        while (it.getSize() < finalSize) {
            REQUIRE(it.peek() == Opcode::kXorr);
            JIT::Reg target, a, b;
            REQUIRE(it.xorr(target, a, b));
            moves.erase(target);
        }
        CHECK(moves.empty());
    }

    SUBCASE("Three Cycle") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{0, 2}, {1, 0}, {2, 1}});
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

}