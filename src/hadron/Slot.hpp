#ifndef SRC_COMPILER_INCLUDE_HADRON_SLOT_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_SLOT_HPP_

#include "hadron/Type.hpp"

#include <cassert>
#include <cstddef>
#include <string>

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

    // Lower 3 bits of pointer bits are unused because all pointers in Hadron are Slot-aligned on 8 bytes.
    enum PointerType : uint64_t {
        kObject = 0,
        kCharArray = 1,
        kInt8Array = 2,
        kInt16Array = 3,
        kInt32Array = 4,
        kFloatArray = 5,
        kDoubleArray = 6,
        kSymbolArray = 7,
        kPointerTypeMask = 7
    };

    inline Slot(void* pointer, PointerType pointerType = PointerType::kObject) {
        assert((reinterpret_cast<uint64_t>(pointer) & kPointerTag) == 0);
        m_asBits = reinterpret_cast<uint64_t>(pointer) | kPointerTag | pointerType;
    }
    // Preserves the lower 48 bits of a 64-bit hash and adds the Hash NaN.
    inline Slot(uint64_t h) { m_asBits = (h & (~kTagMask)) | kSymbolTag; }
    inline Slot(char c) { m_asBits = kCharTag | c; }

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
        case kPointerTag:
            return Type::kObject;
        case kSymbolTag:
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
    inline PointerType getPointerType() const {
        assert((m_asBits & kTagMask) == kPointerTag);
        return static_cast<PointerType>(m_asBits & kPointerTypeMask);
    }
    inline char getChar() const { return m_asBits & (~kTagMask); }


    // Maximum double (quiet NaN with sign bit set without payload):
    //                     seeeeeee|eeeemmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm
    // 0xfff8000000000000: 11111111|11111000|00000000|00000000|00000000|00000000|00000000|00000000
    static constexpr uint64_t kMaxDouble  = 0xfff8000000000000;
    static constexpr uint64_t kNilTag     = 0xfff9000000000000;
    static constexpr uint64_t kInt32Tag   = 0xfffa000000000000;
    static constexpr uint64_t kBooleanTag = 0xfffb000000000000;
    static constexpr uint64_t kPointerTag = 0xfffc000000000000;
    // Lower 48 bits of a 64-bit hash.
    static constexpr uint64_t kSymbolTag  = 0xfffd000000000000;
    static constexpr uint64_t kCharTag    = 0xfffe000000000000;
    static constexpr uint64_t kTagMask    = 0xffff000000000000;

private:
    union {
        double m_asDouble;
        uint64_t m_asBits;
    };
};

static_assert(sizeof(Slot) == 8);
// Making this a signed integer makes for easier pointer arithmetic.
static constexpr int kSlotSize = 8;

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_SLOT_HPP_