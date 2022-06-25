#ifndef SRC_HADRON_LIBRARY_INTEGER_HPP_
#define SRC_HADRON_LIBRARY_INTEGER_HPP_

#include "hadron/Slot.hpp"

namespace hadron {
namespace library {

// Wraps a Slot containing either an int32_t or nil.
class Integer {
public:
    constexpr Integer(): m_slot(Slot::makeNil()) {}
    constexpr Integer(int32_t i): m_slot(Slot::makeInt32(i)) {}
    Integer(Slot i): m_slot(i) { assert(i.isNil() || i.isInt32()); }
    Integer(const Integer& i): m_slot(i.slot()) { assert(m_slot.isNil() || m_slot.isInt32()); }
    ~Integer() = default;

    static inline Integer wrapUnsafe(Slot s) { return Integer(s); }

    inline const Integer& operator=(const Integer& i) { m_slot = i.slot(); return *this; }
    inline bool isNil() const { return m_slot.isNil(); }
    inline int32_t int32() const { return m_slot.getInt32(); }
    inline void setInt32(int32_t i) { m_slot = Slot::makeInt32(i); }
    inline Slot slot() const { return m_slot; }

    inline bool operator==(const Integer& i) const { return m_slot == i.m_slot; }
    inline bool operator!=(const Integer& i) const { return m_slot != i.m_slot; }
    explicit inline operator bool() const { return !isNil(); }

private:
    Slot m_slot;
};

// HIR uses plain Integers as unique identifiers for values. We use the HIRId alias to help clarify when we are
// referring to HIRId values instead of some other Integer identifier.
using BlockId = Integer;
using HIRId = Integer;
using LabelId = Integer;
using Reg = Integer;

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_INTEGER_HPP_