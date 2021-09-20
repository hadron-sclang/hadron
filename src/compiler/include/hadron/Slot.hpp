#ifndef SRC_COMPILER_INCLUDE_HADRON_SLOT_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_SLOT_HPP_

#include "hadron/Type.hpp"

#include <cassert>
#include <cstddef>
#include <string>

static constexpr uint64_t kTagMask = 0xffff000000000000;

// Tagged pointer/double 8-byte Slot structure. Uses code and techniques borrowed from:
// https://www.npopov.com/2012/02/02/Pointer-magic-for-efficient-dynamic-value-representations.html
// by Nikita Popov.
namespace hadron {

class Slot {
public:
    // Construct a Slot with nil value.
    inline Slot() { m_asBits = kNilTag; }
    inline Slot(double number) {
        m_asDouble = number;
        assert(m_asBits < kMaxDouble);
    }
    inline Slot(int32_t number) { m_asBits = number | kInt32Tag; }
    inline Slot(bool value) { m_asBits = (value ? 1 : 0) | kBooleanTag; }
    inline Slot(void* pointer) {
        assert((reinterpret_cast<uint64_t>(pointer) & kObjectTag) == 0);
        m_asBits = reinterpret_cast<uint64_t>(pointer) | kObjectTag;
    }
    // Preserves the lower 48 bits of a 64-bit hash and adds the Hash NaN.
    inline Slot(uint64_t h) { m_asBits = (h & (~kTagMask)) | kHashTag; }

    inline bool operator==(const Slot& s) const { return m_asBits == s.m_asBits; }
    inline bool operator!=(const Slot& s) const { return m_asBits != s.m_asBits; }

    Type getType() const {
        if (m_asBits < kMaxDouble) {
            return Type::kFloat;
        }

        switch (m_asBits & kTagMask) {
        case kNilTag:
            return Type::kNil;
        case kInt32Tag:
            return Type::kInteger;
        case kBooleanTag:
            return Type::kBoolean;
        case kObjectTag:
            return Type::kObject;
        case kHashTag:
            return Type::kSymbol;
        default:
            assert(false);
            return Type::kNil;
        }
    }

    inline int32_t getInt32() const { return static_cast<int32_t>(m_asBits & (~kTagMask)); }
    inline double getFloat() const { return m_asDouble; }
    inline uint64_t getHash() const { return m_asBits & (~kTagMask); }
    inline bool getBool() const { return m_asBits & (~kTagMask); }
    inline void* getPointer() const { return reinterpret_cast<void*>(m_asBits & (~kTagMask)); }

private:
    union {
        double m_asDouble;
        uint64_t m_asBits;
    };

    // Maximum double (quiet NaN with sign bit set without payload):
    //                     seeeeeee|eeeemmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm
    // 0xfff8000000000000: 11111111|11111000|00000000|00000000|00000000|00000000|00000000|00000000
    static constexpr uint64_t kMaxDouble  = 0xfff8000000000000;
    static constexpr uint64_t kNilTag     = 0xfff9000000000000;
    static constexpr uint64_t kInt32Tag   = 0xfffa000000000000;
    static constexpr uint64_t kBooleanTag = 0xfffb000000000000;
    static constexpr uint64_t kObjectTag  = 0xfffc000000000000;
    // Lower 48 bits of a 64-bit hash.
    static constexpr uint64_t kHashTag    = 0xfffd000000000000;
};

static_assert(sizeof(Slot) == 8);
static constexpr int kSlotSize = 8;

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_SLOT_HPP_