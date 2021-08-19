#include "hadron/LifetimeInterval.hpp"

#include "doctest/doctest.h"

namespace hadron {

TEST_CASE("LifetimeInterval Ranges") {
    SUBCASE("Non-overlapping ranges") {
        LifetimeInterval lt;
        CHECK(lt.ranges.size() == 0);
        lt.addLiveRange(4, 5);
        REQUIRE(lt.ranges.size() == 1);
        CHECK(lt.ranges.front().from == 4);
        CHECK(lt.ranges.front().to == 5);
        lt.addLiveRange(0, 1);
        REQUIRE(lt.ranges.size() == 2);
        CHECK(lt.ranges.front().from == 0);
        CHECK(lt.ranges.front().to == 1);
        lt.addLiveRange(8, 10);
        REQUIRE(lt.ranges.size() == 3);
        CHECK(lt.ranges.back().from == 8);
        CHECK(lt.ranges.back().to == 10);
        lt.addLiveRange(2, 3);
        REQUIRE(lt.ranges.size() == 4);
        auto second = ++lt.ranges.begin();
        CHECK(second->from == 2);
        CHECK(second->to == 3);
        lt.addLiveRange(6, 7);
        REQUIRE(lt.ranges.size() == 5);
        second = --lt.ranges.end();
        --second;
        CHECK(second->from == 6);
        CHECK(second->to == 7);
    }

    SUBCASE("Complete overlap expansion of range") {
        LifetimeInterval lt;
        lt.addLiveRange(49, 51);
        CHECK(lt.ranges.size() == 1);
        lt.addLiveRange(47, 53);
        REQUIRE(lt.ranges.size() == 1);
        CHECK(lt.ranges.front().from == 47);
        CHECK(lt.ranges.front().to == 53);
        lt.addLiveRange(35, 40);
        lt.addLiveRange(55, 60);
        lt.addLiveRange(25, 30);
        lt.addLiveRange(75, 80);
        CHECK(lt.ranges.size() == 5);
        lt.addLiveRange(1, 100);
        REQUIRE(lt.ranges.size() == 1);
        CHECK(lt.ranges.front().from == 1);
        CHECK(lt.ranges.front().to == 100);
        // Duplicate addition should change nothing.
        lt.addLiveRange(1, 100);
        REQUIRE(lt.ranges.size() == 1);
        CHECK(lt.ranges.front().from == 1);
        CHECK(lt.ranges.front().to == 100);
        // Addition of smaller ranges contained within larger range should change nothing.
        lt.addLiveRange(1, 2);
        lt.addLiveRange(99, 100);
        lt.addLiveRange(49, 51);
        REQUIRE(lt.ranges.size() == 1);
        CHECK(lt.ranges.front().from == 1);
        CHECK(lt.ranges.front().to == 100);
    }

    SUBCASE("Right expansion no overlap") {
        LifetimeInterval lt;
        lt.addLiveRange(0, 5);
        lt.addLiveRange(10, 15);
        lt.addLiveRange(20, 25);
        lt.addLiveRange(30, 35);
        lt.addLiveRange(40, 45);
        CHECK(lt.ranges.size() == 5);

        lt.addLiveRange(13, 17);
        lt.addLiveRange(31, 39);
        lt.addLiveRange(22, 28);
        lt.addLiveRange(40, 50);
        lt.addLiveRange(4, 6);
        REQUIRE(lt.ranges.size() == 5);
        auto it = lt.ranges.begin();
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
        REQUIRE(it == lt.ranges.end());
    }

    SUBCASE("Left expansion no overlap") {
        LifetimeInterval lt;
        lt.addLiveRange(45, 50);
        lt.addLiveRange(35, 40);
        lt.addLiveRange(25, 30);
        lt.addLiveRange(15, 20);
        lt.addLiveRange(5, 10);
        CHECK(lt.ranges.size() == 5);

        lt.addLiveRange(42, 47);
        lt.addLiveRange(31, 39);
        lt.addLiveRange(4, 6);
        lt.addLiveRange(22, 26);
        lt.addLiveRange(13, 17);
        REQUIRE(lt.ranges.size() == 5);
        REQUIRE(lt.ranges.size() == 5);
        auto it = lt.ranges.begin();
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
        REQUIRE(it == lt.ranges.end());
    }

    SUBCASE("Right expansion with overlap") {
        LifetimeInterval lt;
        lt.addLiveRange(0, 5);
        lt.addLiveRange(20, 25);
        lt.addLiveRange(40, 45);
        lt.addLiveRange(60, 65);
        lt.addLiveRange(80, 85);
        CHECK(lt.ranges.size() == 5);

        lt.addLiveRange(2, 50);
        REQUIRE(lt.ranges.size() == 3);
        auto it = lt.ranges.begin();
        CHECK(it->from == 0);
        CHECK(it->to == 50);
        ++it;
        CHECK(it->from == 60);
        CHECK(it->to == 65);
        ++it;
        CHECK(it->from == 80);
        CHECK(it->to == 85);
        ++it;
        REQUIRE(it == lt.ranges.end());

        lt.addLiveRange(63, 100);
        REQUIRE(lt.ranges.size() == 2);
        CHECK(lt.ranges.front().from == 0);
        CHECK(lt.ranges.front().to == 50);
        CHECK(lt.ranges.back().from == 60);
        CHECK(lt.ranges.back().to == 100);

        lt.addLiveRange(25, 75);
        REQUIRE(lt.ranges.size() == 1);
        CHECK(lt.ranges.front().from == 0);
        CHECK(lt.ranges.front().to == 100);
    }

    SUBCASE("Left expansion with overlap") {
        LifetimeInterval lt;
        lt.addLiveRange(90, 95);
        lt.addLiveRange(70, 75);
        lt.addLiveRange(50, 55);
        lt.addLiveRange(30, 35);
        lt.addLiveRange(10, 15);
        CHECK(lt.ranges.size() == 5);

        lt.addLiveRange(52, 100);
        REQUIRE(lt.ranges.size() == 3);
        auto it = lt.ranges.begin();
        CHECK(it->from == 10);
        CHECK(it->to == 15);
        ++it;
        CHECK(it->from == 30);
        CHECK(it->to == 35);
        ++it;
        CHECK(it->from == 50);
        CHECK(it->to == 100);
        ++it;
        REQUIRE(it == lt.ranges.end());

        lt.addLiveRange(1, 32);
        REQUIRE(lt.ranges.size() == 2);
        CHECK(lt.ranges.front().from == 1);
        CHECK(lt.ranges.front().to == 35);
        CHECK(lt.ranges.back().from == 50);
        CHECK(lt.ranges.back().to == 100);

        lt.addLiveRange(34, 51);
        REQUIRE(lt.ranges.size() == 1);
        CHECK(lt.ranges.front().from == 1);
        CHECK(lt.ranges.front().to == 100);
    }

    SUBCASE("Contiguous regions") {
        LifetimeInterval lt;
        lt.addLiveRange(2, 3);
        lt.addLiveRange(0, 1);
        lt.addLiveRange(4, 5);
        lt.addLiveRange(1, 2);
        lt.addLiveRange(3, 4);
        REQUIRE(lt.ranges.size() == 5);

        auto it = lt.ranges.begin();
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
        REQUIRE(it == lt.ranges.end());

        lt.addLiveRange(1, 3);
        lt.addLiveRange(3, 5);
        REQUIRE(lt.ranges.size() == 3);
        it = lt.ranges.begin();
        CHECK(it->from == 0);
        CHECK(it->to == 1);
        ++it;
        CHECK(it->from == 1);
        CHECK(it->to == 3);
        ++it;
        CHECK(it->from == 3);
        CHECK(it->to == 5);
        ++it;
        REQUIRE(it == lt.ranges.end());

        lt.addLiveRange(0, 5);
        REQUIRE(lt.ranges.size() == 1);
        CHECK(lt.ranges.front().from == 0);
        CHECK(lt.ranges.front().to == 5);
    }
}

} // namespace hadron