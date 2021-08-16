#include "hadron/Lifetime.hpp"

#include "doctest/doctest.h"

namespace hadron {

TEST_CASE("Lifetime Ranges") {
    SUBCASE("Non-overlapping ranges") {
        Lifetime lt;
        CHECK(lt.intervals.size() == 0);
        lt.addInterval(4, 5);
        REQUIRE(lt.intervals.size() == 1);
        CHECK(lt.intervals.front().from == 4);
        CHECK(lt.intervals.front().to == 5);
        lt.addInterval(0, 1);
        CHECK(lt.intervals.size() == 2);
        CHECK(lt.intervals.front().from == 0);
        CHECK(lt.intervals.front().to == 1);
        lt.addInterval(8, 10);
        CHECK(lt.intervals.size() == 3);
        CHECK(lt.intervals.back().from == 8);
        CHECK(lt.intervals.back().to == 10);
        lt.addInterval(2, 3);
        CHECK(lt.intervals.size() == 4);
        auto second = lt.intervals.begin();
        ++second;
        CHECK(second->from == 2);
        CHECK(second->to == 3);
    }
}

} // namespace hadron