#ifndef SRC_COMPILER_INCLUDE_HADRON_SLOT_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_SLOT_HPP_

#include "hadron/Hash.hpp"

#include <functional>
#include <cassert>
#include <cstddef>
#include <string>

// Tagged pointer/double 8-byte Slot structure. Uses code and techniques borrowed from:
// https://www.npopov.com/2012/02/02/Pointer-magic-for-efficient-dynamic-value-representations.html
// by Nikita Popov.
namespace hadron {

struct ObjectHeader;

// These are deliberately independent bits to allow for quick aggregate type comparisons, such as
// type & (kInteger | kFloat) to determine if a type is numeric or
// type & (kString | kSymbol) for character types, etc.
enum TypeFlags : std::int32_t {
    kNoFlags     = 0x00,
    kNilFlag     = 0x01,
    kIntegerFlag = 0x02,
    kFloatFlag   = 0x04,
    kBooleanFlag = 0x08,
    kCharFlag    = 0x10,
    kSymbolFlag  = 0x20,
    kObjectFlag  = 0x40,
    kAllFlags    = 0x7f
};

class Slot {
public:
    Slot(const Slot& s) { m_asBits = s.m_asBits; }
    inline Slot operator=(const Slot& s) { m_asBits = s.m_asBits; return *this; }
    ~Slot() = default;

    // TODO: could these be constexpr? And rename to fromFloat(), 'make' is confusing.
    static inline Slot makeFloat(double d) { return Slot(d); }
    static inline Slot makeNil() { return Slot(kPointerTag); }
    static inline Slot makeInt32(int32_t i) { return Slot(kInt32Tag | (static_cast<uint64_t>(i) & (~kTagMask))); }
    static inline Slot makeBool(bool b) { return Slot(kBooleanTag | (b ? 1ull : 0ull)); }
    static inline Slot makePointer(const void* p) { return Slot(kPointerTag | reinterpret_cast<uint64_t>(p)); }
    static inline Slot makeHash(Hash h) { return Slot(kHashTag | (h & (~kTagMask))); }
    static inline Slot makeChar(char c) { return Slot(kCharTag | c); }

    inline bool operator==(const Slot& s) const { return m_asBits == s.m_asBits; }
    inline bool operator!=(const Slot& s) const { return m_asBits != s.m_asBits; }

    TypeFlags getType() const {
        if (m_asBits < kMaxDouble) {
            return TypeFlags::kFloatFlag;
        }

        switch (m_asBits & kTagMask) {
        case kInt32Tag:
            return TypeFlags::kIntegerFlag;
        case kBooleanTag:
            return TypeFlags::kBooleanFlag;
        case kPointerTag:
            return m_asBits == kPointerTag ? TypeFlags::kNilFlag : TypeFlags::kObjectFlag;
        case kHashTag:
            return TypeFlags::kSymbolFlag;
        case kCharTag:
            return TypeFlags::kCharFlag;
        default:
            assert(false);
            return TypeFlags::kNoFlags;
        }
    }

    inline bool isFloat() const { return m_asBits < kMaxDouble; }
    inline bool isNil() const { return m_asBits == kPointerTag; }
    inline bool isInt32() const { return (m_asBits & kTagMask) == kInt32Tag; }
    inline bool isBool() const { return (m_asBits & kTagMask) == kBooleanTag; }
    inline bool isPointer() const { return ((m_asBits & kTagMask) == kPointerTag) && (m_asBits != kPointerTag); }
    inline bool isHash() const { return (m_asBits & kTagMask) == kHashTag; }
    inline bool isChar() const { return (m_asBits & kTagMask) == kCharTag; }

    inline double getFloat() const { assert(isFloat()); return m_asDouble; }
    inline int32_t getInt32() const { assert(isInt32()); return static_cast<int32_t>(m_asBits & (~kTagMask)); }
    inline bool getBool() const { assert(isBool()); return m_asBits & (~kTagMask); }
    inline ObjectHeader* getPointer() const {
        assert(isPointer());
        return reinterpret_cast<ObjectHeader*>(m_asBits & (~kTagMask));
    }
    inline uint64_t getHash() const { assert(isHash()); return m_asBits & (~kTagMask); }
    inline char getChar() const { assert(isChar()); return m_asBits & (~kTagMask); }

    // Maximum double (quiet NaN with sign bit set without payload):
    //                     seeeeeee|eeeemmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm
    // 0xfff8000000000000: 11111111|11111000|00000000|00000000|00000000|00000000|00000000|00000000
    static constexpr uint64_t kMaxDouble  = 0xfff8000000000000;
    static constexpr uint64_t kInt32Tag   = kMaxDouble;
    static constexpr uint64_t kBooleanTag = 0xfff9000000000000;
    static constexpr uint64_t kPointerTag = 0xfffa000000000000;
    static constexpr uint64_t kHashTag    = 0xfffb000000000000;
    static constexpr uint64_t kCharTag    = 0xfffc000000000000;
    static constexpr uint64_t kTagMask    = 0xffff000000000000;

    // For debugging, normal access should use the get*() methods.
    inline uint64_t asBits() const { return m_asBits; }

private:
    explicit Slot(double floatValue) { m_asDouble = floatValue; }
    explicit Slot(uint64_t bitsValue) { m_asBits = bitsValue; }

    union {
        double m_asDouble;
        uint64_t m_asBits;
    };
};

static_assert(sizeof(Slot) == 8);
// Making this a signed integer makes for easier pointer arithmetic.
static constexpr int kSlotSize = 8;

} // namespace hadron

namespace std {
template<>
struct hash<hadron::Slot> {
    size_t operator()(const hadron::Slot& s) const {
        return static_cast<size_t>(s.getHash());
    }
};
} // namespace std

#endif // SRC_COMPILER_INCLUDE_HADRON_SLOT_HPP_