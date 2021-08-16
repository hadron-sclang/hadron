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
        REQUIRE(lt.intervals.size() == 2);
        CHECK(lt.intervals.front().from == 0);
        CHECK(lt.intervals.front().to == 1);
        lt.addInterval(8, 10);
        REQUIRE(lt.intervals.size() == 3);
        CHECK(lt.intervals.back().from == 8);
        CHECK(lt.intervals.back().to == 10);
        lt.addInterval(2, 3);
        REQUIRE(lt.intervals.size() == 4);
        auto second = ++lt.intervals.begin();
        CHECK(second->from == 2);
        CHECK(second->to == 3);
        lt.addInterval(6, 7);
        REQUIRE(lt.intervals.size() == 5);
        second = --lt.intervals.end();
        --second;
        CHECK(second->from == 6);
        CHECK(second->to == 7);
    }

    SUBCASE("Complete overlap expansion of range") {
        Lifetime lt;
        lt.addInterval(49, 51);
        CHECK(lt.intervals.size() == 1);
        lt.addInterval(47, 53);
        REQUIRE(lt.intervals.size() == 1);
        CHECK(lt.intervals.front().from == 47);
        CHECK(lt.intervals.front().to == 53);
        lt.addInterval(35, 40);
        lt.addInterval(55, 60);
        lt.addInterval(25, 30);
        lt.addInterval(75, 80);
        CHECK(lt.intervals.size() == 5);
        lt.addInterval(1, 100);
        REQUIRE(lt.intervals.size() == 1);
        CHECK(lt.intervals.front().from == 1);
        CHECK(lt.intervals.front().to == 100);
        // Duplicate addition should change nothing.
        lt.addInterval(1, 100);
        REQUIRE(lt.intervals.size() == 1);
        CHECK(lt.intervals.front().from == 1);
        CHECK(lt.intervals.front().to == 100);
        // Addition of smaller ranges contained within larger range should change nothing.
        lt.addInterval(1, 2);
        lt.addInterval(99, 100);
        lt.addInterval(49, 51);
        REQUIRE(lt.intervals.size() == 1);
        CHECK(lt.intervals.front().from == 1);
        CHECK(lt.intervals.front().to == 100);
    }

    SUBCASE("Right expansion no overlap") {
        Lifetime lt;
        lt.addInterval(0, 5);
        lt.addInterval(10, 15);
        lt.addInterval(20, 25);
        lt.addInterval(30, 35);
        lt.addInterval(40, 45);
        CHECK(lt.intervals.size() == 5);

        lt.addInterval(13, 17);
        lt.addInterval(31, 39);
        lt.addInterval(22, 28);
        lt.addInterval(40, 50);
        lt.addInterval(4, 6);
        REQUIRE(lt.intervals.size() == 5);
        auto it = lt.intervals.begin();
        CHECK(it->from == 0);
        CHECK(it->to == 6);
        ++it;
        CHECK(it->from == 10);
        CHECK(it->to == 17);
        ++it;
        CHECK(it->from == 20);
        CHECK(it->to == 28);
        ++it;
        CHECK(it->from == 30);
        CHECK(it->to == 39);
        ++it;
        CHECK(it->from == 40);
        CHECK(it->to == 50);
        ++it;
        REQUIRE(it == lt.intervals.end());
    }

    SUBCASE("Left expansion no overlap") {
        Lifetime lt;
        lt.addInterval(45, 50);
        lt.addInterval(35, 40);
        lt.addInterval(25, 30);
        lt.addInterval(15, 20);
        lt.addInterval(5, 10);
        CHECK(lt.intervals.size() == 5);

        lt.addInterval(42, 47);
        lt.addInterval(31, 39);
        lt.addInterval(4, 6);
        lt.addInterval(22, 26);
        lt.addInterval(13, 17);
        REQUIRE(lt.intervals.size() == 5);
        REQUIRE(lt.intervals.size() == 5);
        auto it = lt.intervals.begin();
        CHECK(it->from == 4);
        CHECK(it->to == 10);
        ++it;
        CHECK(it->from == 13);
        CHECK(it->to == 20);
        ++it;
        CHECK(it->from == 22);
        CHECK(it->to == 30);
        ++it;
        CHECK(it->from == 31);
        CHECK(it->to == 40);
        ++it;
        CHECK(it->from == 42);
        CHECK(it->to == 50);
        ++it;
        REQUIRE(it == lt.intervals.end());
    }

    SUBCASE("Right expansion with overlap") {
        Lifetime lt;
        lt.addInterval(0, 5);
        lt.addInterval(20, 25);
        lt.addInterval(40, 45);
        lt.addInterval(60, 65);
        lt.addInterval(80, 85);
        CHECK(lt.intervals.size() == 5);

        lt.addInterval(2, 50);
        REQUIRE(lt.intervals.size() == 3);
        auto it = lt.intervals.begin();
        CHECK(it->from == 0);
        CHECK(it->to == 50);
        ++it;
        CHECK(it->from == 60);
        CHECK(it->to == 65);
        ++it;
        CHECK(it->from == 80);
        CHECK(it->to == 85);
        ++it;
        REQUIRE(it == lt.intervals.end());

        lt.addInterval(63, 100);
        REQUIRE(lt.intervals.size() == 2);
        CHECK(lt.intervals.front().from == 0);
        CHECK(lt.intervals.front().to == 50);
        CHECK(lt.intervals.back().from == 60);
        CHECK(lt.intervals.back().to == 100);

        lt.addInterval(25, 75);
        REQUIRE(lt.intervals.size() == 1);
        CHECK(lt.intervals.front().from == 0);
        CHECK(lt.intervals.front().to == 100);
    }

    SUBCASE("Left expansion with overlap") {
        Lifetime lt;
        lt.addInterval(90, 95);
        lt.addInterval(70, 75);
        lt.addInterval(50, 55);
        lt.addInterval(30, 35);
        lt.addInterval(10, 15);
        CHECK(lt.intervals.size() == 5);

        lt.addInterval(52, 100);
        REQUIRE(lt.intervals.size() == 3);
        auto it = lt.intervals.begin();
        CHECK(it->from == 10);
        CHECK(it->to == 15);
        ++it;
        CHECK(it->from == 30);
        CHECK(it->to == 35);
        ++it;
        CHECK(it->from == 50);
        CHECK(it->to == 100);
        ++it;
        REQUIRE(it == lt.intervals.end());

        lt.addInterval(1, 32);
        REQUIRE(lt.intervals.size() == 2);
        CHECK(lt.intervals.front().from == 1);
        CHECK(lt.intervals.front().to == 35);
        CHECK(lt.intervals.back().from == 50);
        CHECK(lt.intervals.back().to == 100);

        lt.addInterval(34, 51);
        REQUIRE(lt.intervals.size() == 1);
        CHECK(lt.intervals.front().from == 1);
        CHECK(lt.intervals.front().to == 100);
    }

    SUBCASE("Contiguous regions") {
        Lifetime lt;
        lt.addInterval(2, 3);
        lt.addInterval(0, 1);
        lt.addInterval(4, 5);
        lt.addInterval(1, 2);
        lt.addInterval(3, 4);
        REQUIRE(lt.intervals.size() == 5);

        auto it = lt.intervals.begin();
        CHECK(it->from == 0);
        CHECK(it->to == 1);
        ++it;
        CHECK(it->from == 1);
        CHECK(it->to == 2);
        ++it;
        CHECK(it->from == 2);
        CHECK(it->to == 3);
        ++it;
        CHECK(it->from == 3);
        CHECK(it->to == 4);
        ++it;
        CHECK(it->from == 4);
        CHECK(it->to == 5);
        ++it;
        REQUIRE(it == lt.intervals.end());

        lt.addInterval(1, 3);
        lt.addInterval(3, 5);
        REQUIRE(lt.intervals.size() == 3);
        it = lt.intervals.begin();
        CHECK(it->from == 0);
        CHECK(it->to == 1);
        ++it;
        CHECK(it->from == 1);
        CHECK(it->to == 3);
        ++it;
        CHECK(it->from == 3);
        CHECK(it->to == 5);
        ++it;
        REQUIRE(it == lt.intervals.end());

        lt.addInterval(0, 5);
        REQUIRE(lt.intervals.size() == 1);
        CHECK(lt.intervals.front().from == 0);
        CHECK(lt.intervals.front().to == 5);
    }
}

} // namespace hadron