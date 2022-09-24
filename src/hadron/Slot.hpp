#ifndef SRC_COMPILER_INCLUDE_HADRON_SLOT_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_SLOT_HPP_

#include "hadron/Hash.hpp"

#include <functional>
#include <cassert>
#include <cstddef>
#include <type_traits>

// Tagged pointer/double 8-byte Slot structure. Uses code and techniques borrowed from:
// https://www.npopov.com/2012/02/02/Pointer-magic-for-efficient-dynamic-value-representations.html
// by Nikita Popov.
namespace hadron {

namespace library {
struct Schema;
}

// These are deliberately independent bits to allow for quick aggregate type comparisons, such as
// type & (kInteger | kFloat) to determine if a type is numeric or
// type & (kString | kSymbol) for character types, etc.
enum TypeFlags : std::int32_t {
    kNoFlags = 0x00,
    kNilFlag = 0x01,
    kIntegerFlag = 0x02,
    kFloatFlag = 0x04,
    kBooleanFlag = 0x08,
    kCharFlag = 0x10,
    kSymbolFlag = 0x20,
    kObjectFlag = 0x40,
    kRawPointerFlag = 0x80,
    kAllFlags = 0xff
};

class Slot {
public:
    Slot(const Slot& s) { m_bits = s.m_bits; }
    Slot operator=(const Slot& s) {
        m_bits = s.m_bits;
        return *this;
    }
    ~Slot() = default;

    static Slot makeFloat(double d) { return Slot(d); }
    static constexpr Slot makeNil() { return Slot(kObjectPointerTag); }
    static constexpr Slot makeInt32(int32_t i) { return Slot(kInt32Tag | (static_cast<uint64_t>(i) & (~kTagMask))); }
    static constexpr Slot makeBool(bool b) { return Slot(static_cast<uint64_t>(kBooleanTag | (b ? 1ull : 0ull))); }
    static constexpr Slot makePointer(library::Schema* p) {
        return Slot(kObjectPointerTag | reinterpret_cast<uint64_t>(p));
    }
    static constexpr Slot makeSymbol(Hash h) { return Slot(kSymbolTag | (static_cast<uint64_t>(h) & (~kTagMask))); }
    static constexpr Slot makeChar(char c) { return Slot(kCharTag | c); }
    static constexpr Slot makeRawPointer(const int8_t* p) {
        return Slot(kRawPointerTag | reinterpret_cast<uint64_t>(p));
    }
    static constexpr Slot makeFromBits(uint64_t bits) { return Slot(bits); }

    inline bool operator==(const Slot& s) const { return m_bits == s.m_bits; }
    inline bool operator!=(const Slot& s) const { return m_bits != s.m_bits; }
    explicit inline operator bool() const { return !isNil(); }

    TypeFlags getType() const {
        if (m_bits < kMaxDouble) {
            return TypeFlags::kFloatFlag;
        }

        switch (m_bits & kTagMask) {
        case kInt32Tag:
            return TypeFlags::kIntegerFlag;
        case kBooleanTag:
            return TypeFlags::kBooleanFlag;
        case kObjectPointerTag:
            return m_bits == kObjectPointerTag ? TypeFlags::kNilFlag : TypeFlags::kObjectFlag;
        case kSymbolTag:
            return TypeFlags::kSymbolFlag;
        case kCharTag:
            return TypeFlags::kCharFlag;
        case kRawPointerTag:
            return TypeFlags::kRawPointerFlag;
        default:
            assert(false);
            return TypeFlags::kNoFlags;
        }
    }

    inline bool isFloat() const { return m_bits < kMaxDouble; }
    inline bool isNil() const { return m_bits == kObjectPointerTag; }
    inline bool isInt32() const { return (m_bits & kTagMask) == kInt32Tag; }
    inline bool isBool() const { return (m_bits & kTagMask) == kBooleanTag; }
    inline bool isPointer() const {
        return ((m_bits & kTagMask) == kObjectPointerTag) && (m_bits != kObjectPointerTag);
    }
    inline bool isSymbolHash() const { return (m_bits & kTagMask) == kSymbolTag; }
    inline bool isChar() const { return (m_bits & kTagMask) == kCharTag; }
    inline bool isRawPointer() const { return (m_bits & kTagMask) == kRawPointerTag; }

    inline double getFloat() const {
        assert(isFloat());
        double d = *(reinterpret_cast<const double*>(&m_bits));
        return d;
    }
    inline int32_t getInt32() const {
        assert(isInt32());
        return static_cast<int32_t>(m_bits & (~kTagMask));
    }
    inline bool getBool() const {
        assert(isBool());
        return m_bits & (~kTagMask);
    }
    inline library::Schema* getPointer() const {
        assert(isPointer());
        return reinterpret_cast<library::Schema*>(m_bits & (~kTagMask));
    }
    inline Hash getSymbolHash() const {
        assert(isSymbolHash());
        return static_cast<Hash>(m_bits & (~kTagMask));
    }
    inline char getChar() const {
        assert(isChar());
        return m_bits & (~kTagMask);
    }
    inline int8_t* getRawPointer() const {
        assert(isRawPointer());
        return reinterpret_cast<int8_t*>(m_bits & (~kTagMask));
    }
    inline Slot slot() const { return *this; }

    // Maximum double (quiet NaN with sign bit set without payload):
    //                     seeeeeee|eeeemmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm
    // 0xfff8000000000000: 11111111|11111000|00000000|00000000|00000000|00000000|00000000|00000000
    static constexpr uint64_t kMaxDouble = 0xfff8000000000000;
    static constexpr uint64_t kInt32Tag = kMaxDouble;
    static constexpr uint64_t kBooleanTag = 0xfff9000000000000;
    static constexpr uint64_t kObjectPointerTag = 0xfffa000000000000; // pointer to library::Schema based object
    static constexpr uint64_t kSymbolTag = 0xfffb000000000000;
    static constexpr uint64_t kCharTag = 0xfffc000000000000;
    static constexpr uint64_t kRawPointerTag = 0xfffd000000000000; // not pointing at an object header
    static constexpr uint64_t kForwardingPointerTag = 0xfffe000000000000; // permanently moved, update to new value
    static constexpr uint64_t kTagMask = 0xffff000000000000;

    // For debugging, normal access should use the get*() methods.
    inline uint64_t asBits() const { return m_bits; }

    // Identity hash of objects is only true when the pointers are identical, so for all types contained in a slot a
    // hash of the pointer value.
    inline Hash identityHash(Hash seed = 0) const {
        if (isSymbolHash() && seed == 0) {
            return getSymbolHash();
        }
        return hash(reinterpret_cast<const char*>(this), sizeof(Slot), seed);
    }

private:
    explicit Slot(double floatValue): m_bits(*(uint64_t*)&floatValue) { }
    explicit constexpr Slot(uint64_t bitsValue): m_bits(bitsValue) { }

    uint64_t m_bits;
};

static_assert(sizeof(Slot) == 8);
static_assert(std::is_standard_layout<Slot>::value);
// Making this a signed integer makes for easier pointer arithmetic.
static constexpr int32_t kSlotSize = static_cast<int32_t>(sizeof(Slot));

} // namespace hadron

namespace std {
template <> struct hash<hadron::Slot> {
    size_t operator()(const hadron::Slot& s) const {
        auto hash = s.identityHash();
        return (static_cast<size_t>(hash) << 32) | s.identityHash(hash);
    }
};
} // namespace std

#endif // SRC_COMPILER_INCLUDE_HADRON_SLOT_HPP_