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

TEST_CASE("LifetimeInterval splitAt") {
    SUBCASE("Empty Split") {
        LifetimeInterval lt;
        auto split = lt.splitAt(100);
        CHECK(lt.isEmpty());
        CHECK(split.isEmpty());
    }

    SUBCASE("Split Before") {
        LifetimeInterval lt;
        lt.addLiveRange(10, 20);
        lt.usages.emplace(10);
        lt.addLiveRange(25, 35);
        lt.usages.emplace(25);
        lt.addLiveRange(75, 90);
        lt.usages.emplace(79);
        auto split = lt.splitAt(5);
        CHECK(lt.isEmpty());
        CHECK(lt.usages.empty());
        CHECK(split.start() == 10);
        CHECK(split.end() == 90);
        CHECK(split.ranges.size() == 3);
        CHECK(split.usages.size() == 3);
    }

    SUBCASE("Split First Range") {
        LifetimeInterval lt;
        lt.addLiveRange(4, 7);
        lt.usages.emplace(4);
        lt.usages.emplace(5);
        lt.addLiveRange(9, 12);
        lt.usages.emplace(11);
        lt.addLiveRange(14, 17);
        auto split = lt.splitAt(5);

        CHECK(lt.start() == 4);
        CHECK(lt.end() == 5);
        CHECK(lt.ranges.size() == 1);
        REQUIRE(lt.usages.size() == 1);
        CHECK(*lt.usages.begin() == 4);

        CHECK(split.start() == 5);
        CHECK(split.end() == 17);
        CHECK(split.ranges.size() == 3);
        CHECK(split.usages.size() == 2);
    }

    SUBCASE("Split between ranges") {
        LifetimeInterval lt;
        lt.addLiveRange(10, 15);
        lt.addLiveRange(15, 20);
        auto split = lt.splitAt(15);
        CHECK(lt.start() == 10);
        CHECK(lt.end() == 15);
        CHECK(split.start() == 15);
        CHECK(split.end() == 20);
    }

    SUBCASE("Split After") {
        LifetimeInterval lt;
        lt.addLiveRange(75, 85);
        lt.usages.emplace(80);
        lt.addLiveRange(65, 71);
        lt.usages.emplace(70);
        lt.addLiveRange(35, 37);
        lt.usages.emplace(35);
        auto split = lt.splitAt(90);
        CHECK(split.isEmpty());
        CHECK(split.usages.empty());
        CHECK(lt.start() == 35);
        CHECK(lt.end() == 85);
        CHECK(lt.ranges.size() == 3);
        CHECK(lt.usages.size() == 3);
    }
}

TEST_CASE("LifetimeInterval Covers") {
    SUBCASE("Empty lifetime") {
        LifetimeInterval lt;
        CHECK(!lt.covers(0));
        CHECK(!lt.covers(100));
    }

    SUBCASE("Single lifetime") {
        LifetimeInterval lt;
        lt.addLiveRange(25, 35);
        CHECK(!lt.covers(0));
        CHECK(!lt.covers(1));
        CHECK(lt.covers(25));
        CHECK(lt.covers(30));
        CHECK(lt.covers(34));
        CHECK(!lt.covers(35));
        CHECK(!lt.covers(400));
    }

    SUBCASE("Lifetime holes") {
        LifetimeInterval lt;
        lt.addLiveRange(2, 4);
        lt.addLiveRange(6, 8);
        lt.addLiveRange(10, 12);
        CHECK(!lt.covers(0));
        CHECK(!lt.covers(1));
        CHECK(lt.covers(2));
        CHECK(lt.covers(3));
        CHECK(!lt.covers(4));
        CHECK(!lt.covers(5));
        CHECK(lt.covers(6));
        CHECK(lt.covers(7));
        CHECK(!lt.covers(8));
        CHECK(!lt.covers(9));
        CHECK(lt.covers(10));
        CHECK(lt.covers(11));
        CHECK(!lt.covers(12));
        CHECK(!lt.covers(13));
    }
}

TEST_CASE("LifetimeInterval findFirstIntersection") {
    SUBCASE("Non-intersecting lifetimes") {
        LifetimeInterval lt1;
        LifetimeInterval lt2;
        size_t first = 67;
        CHECK(!lt1.findFirstIntersection(lt2, first));
        CHECK(!lt2.findFirstIntersection(lt1, first));

        lt1.addLiveRange(0, 10);
        CHECK(!lt1.findFirstIntersection(lt2, first));
        CHECK(!lt2.findFirstIntersection(lt1, first));

        lt2.addLiveRange(100, 110);
        CHECK(!lt1.findFirstIntersection(lt2, first));
        CHECK(!lt2.findFirstIntersection(lt1, first));

        lt1.addLiveRange(50, 60);
        CHECK(!lt1.findFirstIntersection(lt2, first));
        CHECK(!lt2.findFirstIntersection(lt1, first));

        lt2.addLiveRange(150, 160);
        CHECK(!lt1.findFirstIntersection(lt2, first));
        CHECK(!lt2.findFirstIntersection(lt1, first));

        lt1.addLiveRange(90, 100);
        CHECK(!lt1.findFirstIntersection(lt2, first));
        CHECK(!lt2.findFirstIntersection(lt1, first));

        lt2.addLiveRange(190, 200);
        CHECK(!lt1.findFirstIntersection(lt2, first));
        CHECK(!lt2.findFirstIntersection(lt1, first));

        // Value should be unchanged.
        CHECK(first == 67);
    }

    SUBCASE("Single range vs multi range") {
        LifetimeInterval single;
        single.addLiveRange(45, 55);

        LifetimeInterval left;
        left.addLiveRange(50, 51);
        left.addLiveRange(52, 53);
        left.addLiveRange(75, 90);
        size_t first = 0;
        CHECK(single.findFirstIntersection(left, first));
        CHECK(first == 50);
        first = 0;
        CHECK(left.findFirstIntersection(single, first));
        CHECK(first == 50);

        LifetimeInterval middle;
        middle.addLiveRange(10, 20);
        middle.addLiveRange(40, 50);
        middle.addLiveRange(60, 75);
        first = 0;
        CHECK(single.findFirstIntersection(middle, first));
        CHECK(first == 45);
        first = 0;
        CHECK(middle.findFirstIntersection(single, first));
        CHECK(first == 45);

        LifetimeInterval right;
        right.addLiveRange(5, 10);
        right.addLiveRange(35, 45);
        right.addLiveRange(54, 199);
        first = 0;
        CHECK(single.findFirstIntersection(right, first));
        CHECK(first == 54);
        first = 0;
        CHECK(right.findFirstIntersection(single, first));
        CHECK(first == 54);

        LifetimeInterval hole;
        hole.addLiveRange(0, 45);
        hole.addLiveRange(55, 100);
        first = 0;
        CHECK(!single.findFirstIntersection(hole, first));
        CHECK(first == 0);
        CHECK(!hole.findFirstIntersection(single, first));
        CHECK(first == 0);
    }
}

} // namespace hadron