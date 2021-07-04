#ifndef SRC_RUNTIME_SLOT_HPP_
#define SRC_RUNTIME_SLOT_HPP_

namespace hadron {

class Slot {
public:
    Slot() = default;
    ~Slot() = default;

    int asInt() const { return m_intValue; }

private:
    int m_intValue = 0;
};

} // namespace hadron

extern "C" {
    int rt_Slot_asInt(const hadron::Slot* slot);
} // extern "C"

#endif // SRC_RUNTIME_SLOT_HPP_