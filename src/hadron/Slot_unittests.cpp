#include "hadron/Slot.hpp"

#include "hadron/Hash.hpp"

#include "doctest/doctest.h"
#include "spdlog/spdlog.h"

#include <limits>
#include <memory>

namespace hadron {

TEST_CASE("Slot float serialization") {
    SUBCASE("simple values") {
        Slot sPos = Slot::makeFloat(1.1);
        REQUIRE(sPos.isFloat());
        CHECK_EQ(sPos.getFloat(), 1.1);

        Slot sZero = Slot::makeFloat(0.0);
        REQUIRE(sZero.isFloat());
        CHECK_EQ(sZero.getFloat(), 0.0);

        Slot sNeg = Slot::makeFloat(-2.2);
        REQUIRE(sNeg.isFloat());
        CHECK_EQ(sNeg.getFloat(), -2.2);
    }

    SUBCASE("maximum/minimum values") {
        Slot sMax = Slot::makeFloat(std::numeric_limits<double>::max());
        REQUIRE(sMax.isFloat());
        CHECK_EQ(sMax.getFloat(), std::numeric_limits<double>::max());

        Slot sMin = Slot::makeFloat(std::numeric_limits<double>::min());
        REQUIRE(sMin.isFloat());
        CHECK_EQ(sMin.getFloat(), std::numeric_limits<double>::min());
    }

    SUBCASE("float equality") {
        Slot s1 = Slot::makeFloat(25.0);
        Slot s2 = Slot::makeFloat(25.1);
        Slot s3 = Slot::makeFloat(25.0);
        CHECK_EQ(s1, s1);
        CHECK_NE(s1, s2);
        CHECK_EQ(s1, s3);
        CHECK_NE(s2, s1);
        CHECK_EQ(s2, s2);
        CHECK_NE(s2, s3);
        CHECK_EQ(s3, s3);
        CHECK_NE(s3, s2);
        CHECK_EQ(s3, s3);

        CHECK_NE(s1, Slot::makeInt32(25));
        CHECK_EQ(Slot::makeFloat(-1.0), Slot::makeFloat(-1.0));
    }
}

TEST_CASE("Slot nil serialization") {
    SUBCASE("nil") {
        Slot sNil = Slot::makeNil();
        REQUIRE(sNil.isNil());
        CHECK_EQ(sNil, Slot::makeNil());
        CHECK_NE(sNil, Slot::makeInt32(0));
        CHECK_NE(sNil, Slot::makeBool(false));
        CHECK_NE(sNil, Slot::makeFloat(0.0));
    }
}

TEST_CASE("Slot int serialization") {
    SUBCASE("simple values") {
        Slot sPos = Slot::makeInt32(1);
        REQUIRE(sPos.isInt32());
        CHECK_EQ(sPos.getInt32(), 1);

        Slot sZero = Slot::makeInt32(0);
        REQUIRE(sZero.isInt32());
        CHECK_EQ(sZero.getInt32(), 0);

        Slot sNeg = Slot::makeInt32(-1);
        REQUIRE(sNeg.isInt32());
        CHECK_EQ(sNeg.getInt32(), -1);
    }

    SUBCASE("maximum/minimum values") {
        Slot sMax = Slot::makeInt32(std::numeric_limits<int32_t>::max());
        REQUIRE(sMax.isInt32());
        CHECK_EQ(sMax.getInt32(), std::numeric_limits<int32_t>::max());

        Slot sMin = Slot::makeInt32(std::numeric_limits<int32_t>::min());
        REQUIRE(sMin.isInt32());
        CHECK_EQ(sMin.getInt32(), std::numeric_limits<int32_t>::min());
    }

    SUBCASE("int equality") {
        Slot s1 = Slot::makeInt32(-17);
        Slot s2 = Slot::makeInt32(18);
        Slot s3 = Slot::makeInt32(-17);
        CHECK_EQ(s1, s1);
        CHECK_NE(s1, s2);
        CHECK_EQ(s1, s3);
        CHECK_NE(s2, s1);
        CHECK_EQ(s2, s2);
        CHECK_NE(s2, s3);
        CHECK_EQ(s3, s3);
        CHECK_NE(s3, s2);
        CHECK_EQ(s3, s3);
    }
}

TEST_CASE("Slot bool serialization") {
    SUBCASE("bool") {
        Slot sTrue = Slot::makeBool(true);
        REQUIRE(sTrue.isBool());
        CHECK_EQ(sTrue.getBool(), true);
        Slot sFalse = Slot::makeBool(false);
        REQUIRE(sFalse.isBool());
        CHECK_EQ(sFalse.getBool(), false);
        CHECK_NE(sTrue, sFalse);
        CHECK_EQ(sTrue, Slot::makeBool(true));
        CHECK_EQ(sFalse, Slot::makeBool(false));
    }
}

TEST_CASE("Slot pointer serialization") {
    SUBCASE("pointer values") {
        std::unique_ptr<uint8_t[]> pointer = std::make_unique<uint8_t[]>(16);
        Slot p = Slot::makePointer(pointer.get());
        REQUIRE(p.isPointer());
        REQUIRE_EQ(reinterpret_cast<uint8_t*>(p.getPointer()), pointer.get());
        pointer[3] = 0xff;
        CHECK_EQ(reinterpret_cast<uint8_t*>(p.getPointer())[3], 0xff);
    }

    SUBCASE("null pointers are nil") {
        Slot p = Slot::makePointer(nullptr);
        REQUIRE(p.isNil());
        CHECK_EQ(p, Slot::makeNil());
    }
}

TEST_CASE("Slot hash serialization") {
    SUBCASE("hash") {
        const char* testInput = "test input string";
        const char* otherTestInput = "should have a different hash";
        Slot h = Slot::makeHash(hash(testInput));
        REQUIRE(h.isHash());
        CHECK_EQ(h.getHash(), hash(testInput));
        CHECK_NE(h.getHash(), hash(otherTestInput));
        Slot h2 = Slot::makeHash(hash(testInput));
        REQUIRE(h2.isHash());
        CHECK_EQ(h, h2);
    }
}

TEST_CASE("Slot char serialization") {
    SUBCASE("char") {
        Slot c = Slot::makeChar('$');
        REQUIRE(c.isChar());
        CHECK_EQ(c.getChar(), '$');
    }
}

} // namespace hadron