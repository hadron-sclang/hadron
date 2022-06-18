#ifndef SRC_HADRON_LIBRARY_BOOLEAN_HPP_
#define SRC_HADRON_LIBRARY_BOOLEAN_HPP_

#include "hadron/Slot.hpp"

namespace hadron {
namespace library {

class Boolean {
public:
    Boolean(): m_slot(Slot::makeNil()) {}
    Boolean(bool b): m_slot(Slot::makeBool(b)) {}
    Boolean(Slot b): m_slot(b) { assert(b.isNil() || b.isBool()); }
    Boolean(const Boolean& b): m_slot(b.slot()) { assert(m_slot.isNil() || m_slot.isBool()); }

    static inline Boolean wrapUnsafe(Slot s) { return Boolean(s); }

    inline const Boolean& operator=(const Boolean& b) { m_slot = b.slot(); return *this; }
    inline bool isNil() const { return m_slot.isNil(); }
    inline bool boolean() const { return m_slot.getBool(); }
    inline void setBool(bool b) { m_slot = Slot::makeBool(b); }
    inline Slot slot() const { return m_slot; }

    inline bool operator==(const Boolean& i) const { return m_slot == i.m_slot; }
    inline bool operator!=(const Boolean& i) const { return m_slot != i.m_slot; }
    explicit inline operator bool() const { return !isNil(); }

private:
    Slot m_slot;
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_BOOLEAN_HPP_