#include "hadron/library/HadronLifetimeInterval.hpp"

#include "hadron/library/LibraryTestFixture.hpp"

#include "doctest/doctest.h"

namespace hadron {

TEST_CASE_FIXTURE(LibraryTestFixture, "LifetimeInterval") {
    SUBCASE("addLiveRange") {
        SUBCASE("Non-overlapping ranges") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            CHECK(lt.ranges().size() == 0);
            lt.addLiveRange(context(), 4, 5);
            REQUIRE(lt.ranges().size() == 1);
            CHECK(lt.ranges().typedFirst().from().int32() == 4);
            CHECK(lt.ranges().typedFirst().to().int32() == 5);
            lt.addLiveRange(context(), 0, 1);
            REQUIRE(lt.ranges().size() == 2);
            CHECK(lt.ranges().typedFirst().from().int32() == 0);
            CHECK(lt.ranges().typedFirst().to().int32() == 1);
            lt.addLiveRange(context(), 8, 10);
            REQUIRE(lt.ranges().size() == 3);
            CHECK(lt.ranges().typedLast().from().int32() == 8);
            CHECK(lt.ranges().typedLast().to().int32() == 10);
            lt.addLiveRange(context(), 2, 3);
            REQUIRE(lt.ranges().size() == 4);
            auto second = lt.ranges().typedAt(1);
            CHECK_EQ(second.from().int32(), 2);
            CHECK_EQ(second.to().int32(), 3);
            lt.addLiveRange(context(), 6, 7);
            REQUIRE(lt.ranges().size() == 5);
            second = lt.ranges().typedAt(lt.ranges().size() - 2);
            CHECK_EQ(second.from().int32(), 6);
            CHECK_EQ(second.to().int32(), 7);
        }

        SUBCASE("Complete overlap expansion of range") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            lt.addLiveRange(context(), 49, 51);
            CHECK(lt.ranges().size() == 1);
            lt.addLiveRange(context(), 47, 53);
            REQUIRE(lt.ranges().size() == 1);
            CHECK(lt.ranges().typedFirst().from().int32() == 47);
            CHECK(lt.ranges().typedFirst().to().int32() == 53);
            lt.addLiveRange(context(), 35, 40);
            lt.addLiveRange(context(), 55, 60);
            lt.addLiveRange(context(), 25, 30);
            lt.addLiveRange(context(), 75, 80);
            CHECK(lt.ranges().size() == 5);
            lt.addLiveRange(context(), 1, 100);
            REQUIRE(lt.ranges().size() == 1);
            CHECK(lt.ranges().typedFirst().from().int32() == 1);
            CHECK(lt.ranges().typedFirst().to().int32() == 100);
            // Duplicate addition should change nothing.
            lt.addLiveRange(context(), 1, 100);
            REQUIRE(lt.ranges().size() == 1);
            CHECK(lt.ranges().typedFirst().from().int32() == 1);
            CHECK(lt.ranges().typedFirst().to().int32() == 100);
            // Addition of smaller ranges contained within larger range should change nothing.
            lt.addLiveRange(context(), 1, 2);
            lt.addLiveRange(context(), 99, 100);
            lt.addLiveRange(context(), 49, 51);
            REQUIRE(lt.ranges().size() == 1);
            CHECK(lt.ranges().typedFirst().from().int32() == 1);
            CHECK(lt.ranges().typedFirst().to().int32() == 100);
        }

        SUBCASE("Right expansion no overlap") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            lt.addLiveRange(context(), 0, 5);
            lt.addLiveRange(context(), 10, 15);
            lt.addLiveRange(context(), 20, 25);
            lt.addLiveRange(context(), 30, 35);
            lt.addLiveRange(context(), 40, 45);
            CHECK(lt.ranges().size() == 5);

            lt.addLiveRange(context(), 13, 17);
            lt.addLiveRange(context(), 31, 39);
            lt.addLiveRange(context(), 22, 28);
            lt.addLiveRange(context(), 40, 50);
            lt.addLiveRange(context(), 4, 6);
            REQUIRE(lt.ranges().size() == 5);
            auto range = lt.ranges().typedAt(0);
            CHECK_EQ(range.from().int32(), 0);
            CHECK_EQ(range.to().int32(), 6);
            range = lt.ranges().typedAt(1);
            CHECK_EQ(range.from().int32(), 10);
            CHECK_EQ(range.to().int32(), 17);
            range = lt.ranges().typedAt(2);
            CHECK_EQ(range.from().int32(), 20);
            CHECK_EQ(range.to().int32(), 28);
            range = lt.ranges().typedAt(3);
            CHECK_EQ(range.from().int32(), 30);
            CHECK_EQ(range.to().int32(), 39);
            range = lt.ranges().typedAt(4);
            CHECK_EQ(range.from().int32(), 40);
            CHECK_EQ(range.to().int32(), 50);
        }

        SUBCASE("Left expansion no overlap") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            lt.addLiveRange(context(), 45, 50);
            lt.addLiveRange(context(), 35, 40);
            lt.addLiveRange(context(), 25, 30);
            lt.addLiveRange(context(), 15, 20);
            lt.addLiveRange(context(), 5, 10);
            CHECK(lt.ranges().size() == 5);

            lt.addLiveRange(context(), 42, 47);
            lt.addLiveRange(context(), 31, 39);
            lt.addLiveRange(context(), 4, 6);
            lt.addLiveRange(context(), 22, 26);
            lt.addLiveRange(context(), 13, 17);
            REQUIRE(lt.ranges().size() == 5);

            auto range = lt.ranges().typedAt(0);
            CHECK_EQ(range.from().int32(), 4);
            CHECK_EQ(range.to().int32(), 10);

            range = lt.ranges().typedAt(1);
            CHECK_EQ(range.from().int32(), 13);
            CHECK_EQ(range.to().int32(), 20);

            range = lt.ranges().typedAt(2);
            CHECK_EQ(range.from().int32(), 22);
            CHECK_EQ(range.to().int32(), 30);

            range = lt.ranges().typedAt(3);
            CHECK_EQ(range.from().int32(), 31);
            CHECK_EQ(range.to().int32(), 40);

            range = lt.ranges().typedAt(4);
            CHECK_EQ(range.from().int32(), 42);
            CHECK_EQ(range.to().int32(), 50);
        }

        SUBCASE("Right expansion with overlap") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            lt.addLiveRange(context(), 0, 5);
            lt.addLiveRange(context(), 20, 25);
            lt.addLiveRange(context(), 40, 45);
            lt.addLiveRange(context(), 60, 65);
            lt.addLiveRange(context(), 80, 85);
            CHECK(lt.ranges().size() == 5);

            lt.addLiveRange(context(), 2, 50);
            REQUIRE(lt.ranges().size() == 3);
            auto range = lt.ranges().typedAt(0);
            CHECK_EQ(range.from().int32(), 0);
            CHECK_EQ(range.to().int32(), 50);
            range = lt.ranges().typedAt(1);
            CHECK_EQ(range.from().int32(), 60);
            CHECK_EQ(range.to().int32(), 65);
            range = lt.ranges().typedAt(2);
            CHECK_EQ(range.from().int32(), 80);
            CHECK_EQ(range.to().int32(), 85);

            lt.addLiveRange(context(), 63, 100);
            REQUIRE(lt.ranges().size() == 2);
            CHECK(lt.ranges().typedFirst().from().int32() == 0);
            CHECK(lt.ranges().typedFirst().to().int32() == 50);
            CHECK(lt.ranges().typedLast().from().int32() == 60);
            CHECK(lt.ranges().typedLast().to().int32() == 100);

            lt.addLiveRange(context(), 25, 75);
            REQUIRE(lt.ranges().size() == 1);
            CHECK(lt.ranges().typedFirst().from().int32() == 0);
            CHECK(lt.ranges().typedFirst().to().int32() == 100);
        }

        SUBCASE("Left expansion with overlap") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            lt.addLiveRange(context(), 90, 95);
            lt.addLiveRange(context(), 70, 75);
            lt.addLiveRange(context(), 50, 55);
            lt.addLiveRange(context(), 30, 35);
            lt.addLiveRange(context(), 10, 15);
            CHECK(lt.ranges().size() == 5);

            lt.addLiveRange(context(), 52, 100);
            REQUIRE(lt.ranges().size() == 3);
            auto range = lt.ranges().typedAt(0);
            CHECK_EQ(range.from().int32(), 10);
            CHECK_EQ(range.to().int32(), 15);

            range = lt.ranges().typedAt(1);
            CHECK_EQ(range.from().int32(), 30);
            CHECK_EQ(range.to().int32(), 35);

            range = lt.ranges().typedAt(2);
            CHECK_EQ(range.from().int32(), 50);
            CHECK_EQ(range.to().int32(), 100);

            lt.addLiveRange(context(), 1, 32);
            REQUIRE(lt.ranges().size() == 2);
            CHECK(lt.ranges().typedFirst().from().int32() == 1);
            CHECK(lt.ranges().typedFirst().to().int32() == 35);
            CHECK(lt.ranges().typedLast().from().int32() == 50);
            CHECK(lt.ranges().typedLast().to().int32() == 100);

            lt.addLiveRange(context(), 34, 51);
            REQUIRE(lt.ranges().size() == 1);
            CHECK(lt.ranges().typedFirst().from().int32() == 1);
            CHECK(lt.ranges().typedFirst().to().int32() == 100);
        }

        SUBCASE("Contiguous regions") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            lt.addLiveRange(context(), 2, 3);
            lt.addLiveRange(context(), 0, 1);
            lt.addLiveRange(context(), 4, 5);
            lt.addLiveRange(context(), 1, 2);
            lt.addLiveRange(context(), 3, 4);
            REQUIRE(lt.ranges().size() == 5);

            auto range = lt.ranges().typedAt(0);
            CHECK_EQ(range.from().int32(), 0);
            CHECK_EQ(range.to().int32(), 1);

            range = lt.ranges().typedAt(1);
            CHECK_EQ(range.from().int32(), 1);
            CHECK_EQ(range.to().int32(), 2);

            range = lt.ranges().typedAt(2);
            CHECK_EQ(range.from().int32(), 2);
            CHECK_EQ(range.to().int32(), 3);

            range = lt.ranges().typedAt(3);
            CHECK_EQ(range.from().int32(), 3);
            CHECK_EQ(range.to().int32(), 4);

            range = lt.ranges().typedAt(4);
            CHECK_EQ(range.from().int32(), 4);
            CHECK_EQ(range.to().int32(), 5);

            lt.addLiveRange(context(), 1, 3);
            lt.addLiveRange(context(), 3, 5);
            REQUIRE(lt.ranges().size() == 3);

            range = lt.ranges().typedAt(0);
            CHECK_EQ(range.from().int32(), 0);
            CHECK_EQ(range.to().int32(), 1);

            range = lt.ranges().typedAt(1);
            CHECK_EQ(range.from().int32(), 1);
            CHECK_EQ(range.to().int32(), 3);

            range = lt.ranges().typedAt(2);
            CHECK_EQ(range.from().int32(), 3);
            CHECK_EQ(range.to().int32(), 5);

            lt.addLiveRange(context(), 0, 5);
            REQUIRE(lt.ranges().size() == 1);
            CHECK(lt.ranges().typedFirst().from().int32() == 0);
            CHECK(lt.ranges().typedFirst().to().int32() == 5);
        }
    } // addLiveRange

    SUBCASE("splitAt") {
        SUBCASE("Empty Split") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            auto split = lt.splitAt(context(), 100);
            CHECK(lt.isEmpty());
            CHECK(split.isEmpty());
        }

        SUBCASE("Split Before") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            lt.addLiveRange(context(), 10, 20);
            lt.usages().add(context(), Slot::makeInt32(10));
            lt.addLiveRange(context(), 25, 35);
            lt.usages().add(context(), Slot::makeInt32(25));
            lt.addLiveRange(context(), 75, 90);
            lt.usages().add(context(), Slot::makeInt32(79));
            auto split = lt.splitAt(context(), 5);
            CHECK(lt.isEmpty());
            CHECK_EQ(lt.usages().size(), 0);
            CHECK_EQ(split.start().int32(), 10);
            CHECK_EQ(split.end().int32(), 90);
            CHECK_EQ(split.ranges().size(), 3);
            CHECK_EQ(split.usages().size(), 3);
        }

        SUBCASE("Split First Range") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            lt.addLiveRange(context(), 4, 7);
            lt.usages().add(context(), Slot::makeInt32(4));
            lt.usages().add(context(), Slot::makeInt32(5));
            lt.addLiveRange(context(), 9, 12);
            lt.usages().add(context(), Slot::makeInt32(11));
            lt.addLiveRange(context(), 14, 17);
            auto split = lt.splitAt(context(), 5);

            CHECK(lt.start() == 4);
            CHECK(lt.end() == 5);
            CHECK(lt.ranges().size() == 1);
            REQUIRE_EQ(lt.usages().size(), 1);
            CHECK_EQ(lt.usages().items().first(), Slot::makeInt32(4));

            CHECK_EQ(split.start().int32(), 5);
            CHECK_EQ(split.end().int32(), 17);
            CHECK(split.ranges().size() == 3);
            CHECK(split.usages().size() == 2);
        }

        SUBCASE("Split between ranges") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            lt.addLiveRange(context(), 10, 15);
            lt.addLiveRange(context(), 15, 20);
            auto split = lt.splitAt(context(), 15);
            CHECK(lt.start() == 10);
            CHECK(lt.end() == 15);
            CHECK(split.start() == 15);
            CHECK(split.end() == 20);
        }

        SUBCASE("Split After") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            lt.addLiveRange(context(), 75, 85);
            lt.usages().add(context(), Slot::makeInt32(80));
            lt.addLiveRange(context(), 65, 71);
            lt.usages().add(context(), Slot::makeInt32(70));
            lt.addLiveRange(context(), 35, 37);
            lt.usages().add(context(), Slot::makeInt32(35));
            auto split = lt.splitAt(context(), 90);
            CHECK(split.isEmpty());
            CHECK_EQ(split.usages().size(), 0);
            CHECK(lt.start() == 35);
            CHECK(lt.end() == 85);
            CHECK(lt.ranges().size() == 3);
            CHECK(lt.usages().size() == 3);
        }
    } // splitAt

    SUBCASE("covers") {
        SUBCASE("Empty lifetime") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            CHECK(!lt.covers(0));
            CHECK(!lt.covers(100));
        }

        SUBCASE("Single lifetime") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            lt.addLiveRange(context(), 25, 35);
            CHECK(!lt.covers(0));
            CHECK(!lt.covers(1));
            CHECK(lt.covers(25));
            CHECK(lt.covers(30));
            CHECK(lt.covers(34));
            CHECK(!lt.covers(35));
            CHECK(!lt.covers(400));
        }

        SUBCASE("Lifetime holes") {
            auto lt = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            lt.addLiveRange(context(), 2, 4);
            lt.addLiveRange(context(), 6, 8);
            lt.addLiveRange(context(), 10, 12);
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
    } // covers

    SUBCASE("findFirstIntersection") {
        SUBCASE("Non-intersecting lifetimes") {
            auto lt1 = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            auto lt2 = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            int32_t first = 67;
            CHECK(!lt1.findFirstIntersection(lt2, first));
            CHECK(!lt2.findFirstIntersection(lt1, first));

            lt1.addLiveRange(context(), 0, 10);
            CHECK(!lt1.findFirstIntersection(lt2, first));
            CHECK(!lt2.findFirstIntersection(lt1, first));

            lt2.addLiveRange(context(), 100, 110);
            CHECK(!lt1.findFirstIntersection(lt2, first));
            CHECK(!lt2.findFirstIntersection(lt1, first));

            lt1.addLiveRange(context(), 50, 60);
            CHECK(!lt1.findFirstIntersection(lt2, first));
            CHECK(!lt2.findFirstIntersection(lt1, first));

            lt2.addLiveRange(context(), 150, 160);
            CHECK(!lt1.findFirstIntersection(lt2, first));
            CHECK(!lt2.findFirstIntersection(lt1, first));

            lt1.addLiveRange(context(), 90, 100);
            CHECK(!lt1.findFirstIntersection(lt2, first));
            CHECK(!lt2.findFirstIntersection(lt1, first));

            lt2.addLiveRange(context(), 190, 200);
            CHECK(!lt1.findFirstIntersection(lt2, first));
            CHECK(!lt2.findFirstIntersection(lt1, first));

            // Value should be unchanged.
            CHECK(first == 67);
        }

        SUBCASE("Single range vs multi range") {
            auto single = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            single.addLiveRange(context(), 45, 55);

            auto left = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            left.addLiveRange(context(), 50, 51);
            left.addLiveRange(context(), 52, 53);
            left.addLiveRange(context(), 75, 90);
            int32_t first = 0;
            CHECK(single.findFirstIntersection(left, first));
            CHECK(first == 50);
            first = 0;
            CHECK(left.findFirstIntersection(single, first));
            CHECK(first == 50);

            auto middle = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            middle.addLiveRange(context(), 10, 20);
            middle.addLiveRange(context(), 40, 50);
            middle.addLiveRange(context(), 60, 75);
            first = 0;
            CHECK(single.findFirstIntersection(middle, first));
            CHECK(first == 45);
            first = 0;
            CHECK(middle.findFirstIntersection(single, first));
            CHECK(first == 45);

            auto right = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            right.addLiveRange(context(), 5, 10);
            right.addLiveRange(context(), 35, 45);
            right.addLiveRange(context(), 54, 199);
            first = 0;
            CHECK(single.findFirstIntersection(right, first));
            CHECK(first == 54);
            first = 0;
            CHECK(right.findFirstIntersection(single, first));
            CHECK(first == 54);

            auto hole = library::LifetimeInterval::makeLifetimeInterval(context(), 0);
            hole.addLiveRange(context(), 0, 45);
            hole.addLiveRange(context(), 55, 100);
            first = 0;
            CHECK(!single.findFirstIntersection(hole, first));
            CHECK(first == 0);
            CHECK(!hole.findFirstIntersection(single, first));
            CHECK(first == 0);
        }
    } // findFirstIntersection
}

} // namespace hadron