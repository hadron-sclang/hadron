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
}

} // namespace hadron