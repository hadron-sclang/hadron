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
        std::array<uint8_t, 16> jitBuffer;
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
        std::array<uint8_t, 16> jitBuffer;
        jit.begin(jitBuffer.data(), jitBuffer.size());
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);
        OpcodeIterator it;
        it.setBuffer(jitBuffer.data(), jitBuffer.size());
        
        // Should have written 3 bytes to the buffer
        REQUIRE_EQ(finalSize, 3);
        CHECK_EQ(jitBuffer[0], Opcode::kMovr);
        CHECK_EQ(jitBuffer[1], 2);
        CHECK_EQ(jitBuffer[2], 3);
    }

    SUBCASE("register to spill") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{0, -1}});
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        size_t finalSize = 0;
        jit.end(&finalSize);
        // Should have written 3 bytes to the buffer
        REQUIRE_EQ(finalSize, 3);
        CHECK_EQ(jitBuffer[0], Opcode::kMovr);
        CHECK_EQ(jitBuffer[1], 2);
        CHECK_EQ(jitBuffer[2], 3);


        REQUIRE(jit.instructions().size() == 1);
        CHECK(jit.instructions()[0] ==
            VirtualJIT::Inst{ VirtualJIT::Opcode::kStxiL, -1 * kSlotSize, JIT::kStackPointerReg, 0 });
    }

    SUBCASE("spill to register") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{-24, 5}});
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        REQUIRE(jit.instructions().size() == 1);
        CHECK(jit.instructions()[0] ==
            VirtualJIT::Inst{ VirtualJIT::Opcode::kLdxiL, 5, JIT::kStackPointerReg, -24 * kSlotSize });
    }

    SUBCASE("multiple independent moves") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{-3, 2}, {9, 7}, {3, -1}});
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        REQUIRE(jit.instructions().size() == 3);
        // Instructions can be in any order so check all three and remove from the map as we encounter.
        for (const auto& inst : jit.instructions()) {
            std::unordered_map<int, int>::iterator it = moves.end();
            if (inst == VirtualJIT::Inst{ VirtualJIT::Opcode::kLdxiL, 2, JIT::kStackPointerReg,
                    -3 * kSlotSize }) {
                it = moves.find(-3);
            } else if (inst == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 7, 9 }) {
                it = moves.find(9);
            } else if (inst == VirtualJIT::Inst{ VirtualJIT::Opcode::kStxiL, -1 * kSlotSize, JIT::kStackPointerReg,
                    3 }) {
                it = moves.find(3);
            }
            REQUIRE(it != moves.end());
            moves.erase(it);
        }
        CHECK(moves.empty());
    }
}

TEST_CASE("MoveScheduler Chains") {
    SUBCASE("Two Chain") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{3, 2}, {2, 1}});
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        REQUIRE(jit.instructions().size() == 2);
        // The 1 <- 2 move needs to happen before the 2 <- 3 move.
        CHECK(jit.instructions()[0] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 1, 2 });
        CHECK(jit.instructions()[1] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 2, 3 });
    }

    SUBCASE("Three Chain Through Spill") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{0, 3}, {3, 1}, {-1, 0}});
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        REQUIRE(jit.instructions().size() == 3);
        CHECK(jit.instructions()[0] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 1, 3 });
        CHECK(jit.instructions()[1] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 3, 0 });
        CHECK(jit.instructions()[2] ==
            VirtualJIT::Inst{ VirtualJIT::Opcode::kLdxiL, 0, JIT::kStackPointerReg, -1 * kSlotSize });
    }

    SUBCASE("Eight Chain Unordered") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{6, 5}, {2, 1}, {5, 4}, {7, 6}, {4, 3}, {1, 0}, {3, 2}, {8, 7}});
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        REQUIRE(jit.instructions().size() == 8);
        CHECK(jit.instructions()[0] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 0, 1 });
        CHECK(jit.instructions()[1] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 1, 2 });
        CHECK(jit.instructions()[2] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 2, 3 });
        CHECK(jit.instructions()[3] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 3, 4 });
        CHECK(jit.instructions()[4] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 4, 5 });
        CHECK(jit.instructions()[5] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 5, 6 });
        CHECK(jit.instructions()[6] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 6, 7 });
        CHECK(jit.instructions()[7] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, 7, 8 });
    }
}

TEST_CASE("MoveScheduler Cycles") {
    SUBCASE("Two Simple Cycles") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{0, 3}, {2, 4}, {4, 2}, {3, 0}});
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        REQUIRE(jit.instructions().size() == 6);
        // Instructions should be all xor operations and every target register should be set at least once.
        for (auto inst : jit.instructions()) {
            CHECK(inst[0] == VirtualJIT::Opcode::kXorr);
            moves.erase(inst[1]);
        }
        CHECK(moves.empty());
    }

    SUBCASE("Three Cycle") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{0, 2}, {1, 0}, {2, 1}});
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        REQUIRE(jit.instructions().size() == 4);
        // First operation should be a store to the temporariy slot.
        REQUIRE(jit.instructions()[0][0] == VirtualJIT::Opcode::kStrL);
        CHECK(jit.instructions()[0][1] == JIT::kStackPointerReg);
        JIT::Reg chainReg = jit.instructions()[0][3];
        // Second two operations are moves within the cycle.
        CHECK(jit.instructions()[1] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr, chainReg, (chainReg + 1) % 3 });
        CHECK(jit.instructions()[2] == VirtualJIT::Inst{ VirtualJIT::Opcode::kMovr,
            (chainReg + 1) % 3, (chainReg + 2) % 3});
        // Last instruction is a load into the last reg in the chain.
        REQUIRE(jit.instructions()[3] == VirtualJIT::Inst{ VirtualJIT::Opcode::kLdrL, (chainReg + 2) % 3,
            JIT::kStackPointerReg });
    }
}

}