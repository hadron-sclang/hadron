#ifndef SRC_HADRON_LIBRARY_INTEGER_HPP_
#define SRC_HADRON_LIBRARY_INTEGER_HPP_

#include "hadron/Slot.hpp"

namespace hadron {
namespace library {

// Wraps a Slot containing either an int32_t or nil.
class Integer {
public:
    Integer(): m_slot(Slot::makeNil()) {}
    Integer(int32_t i): m_slot(Slot::makeInt32(i)) {}
    Integer(Slot i): m_slot(i) { assert(i.isNil() || i.isInt32()); }
    ~Integer() {}

    Integer& operator=(const Integer& i) { m_slot = i.m_slot; return *this; }

    inline bool isNil() const { return m_slot.isNil(); }
    inline int32_t int32() const { return m_slot.getInt32(); }
    void setInt32(int32_t i) { m_slot = Slot::makeInt32(i); }
    inline Slot slot() const { return m_slot; }

    inline bool operator==(const Integer& i) const { return m_slot == i.m_slot; }
    inline bool operator!=(const Integer& i) const { return m_slot != i.m_slot; }
    explicit inline operator bool() const { return !isNil(); }

private:
    Slot m_slot;
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_INTEGER_HPP_