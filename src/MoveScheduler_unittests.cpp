#include "hadron/MoveScheduler.hpp"

#include "hadron/Slot.hpp"
#include "hadron/VirtualJIT.hpp"

#include "doctest/doctest.h"
#include "spdlog/spdlog.h"

#include <unordered_map>

namespace hadron {

TEST_CASE("MoveScheduler simple") {
    SUBCASE("empty set") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves;
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        CHECK(jit.instructions().size() == 0);
    }

    SUBCASE("register to register") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{3, 2}});
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        REQUIRE(jit.instructions().size() == 1);
        CHECK(jit.instructions()[0] == VirtualJIT::Inst{ VirtualJIT::Opcodes::kMovr, 2, 3 });
    }

    SUBCASE("register to spill") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{0, -1}});
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        REQUIRE(jit.instructions().size() == 1);
        CHECK(jit.instructions()[0] ==
            VirtualJIT::Inst{ VirtualJIT::Opcodes::kStxiW, Slot::slotValueOffset(-1), JIT::kStackPointerReg, 0 });
    }

    SUBCASE("spill to register") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{-24, 5}});
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        REQUIRE(jit.instructions().size() == 1);
        CHECK(jit.instructions()[0] ==
            VirtualJIT::Inst{ VirtualJIT::Opcodes::kLdxiW, 5, JIT::kStackPointerReg, Slot::slotValueOffset(-24) });
    }
}

TEST_CASE("MoveScheduler Chains") {
    SUBCASE("Shortest Chain") {
        VirtualJIT jit;
        std::unordered_map<int, int> moves({{3, 2}, {2, 1}});
        MoveScheduler ms;
        REQUIRE(ms.scheduleMoves(moves, &jit));
        REQUIRE(jit.instructions().size() == 2);
        // The 1 <- 2 move needs to happen before the 2 <- 3 move.
        CHECK(jit.instructions()[0] == VirtualJIT::Inst{ VirtualJIT::Opcodes::kMovr, 1, 2 });
        CHECK(jit.instructions()[1] == VirtualJIT::Inst{ VirtualJIT::Opcodes::kMovr, 2, 3 });
    }
}

}