#include "hadron/MoveScheduler.hpp"

#include "hadron/VirtualJIT.hpp"

#include "doctest/doctest.h"

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
}

}