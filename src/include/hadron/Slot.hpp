#ifndef SRC_RUNTIME_SLOT_HPP_
#define SRC_RUNTIME_SLOT_HPP_

#include "hadron/Type.hpp"

#include <cstddef>

namespace hadron {

struct Slot {
public:
    Slot(): type(kNil) {}
    Slot(int intValue): type(Type::kInteger), intValue(intValue) {}
    ~Slot() = default;

    // Placement new
    void* operator new(size_t, Slot* address) { return address; }

    Type type;
    int intValue = 0;
};

} // namespace hadron

extern "C" {
    void* slot_Init(hadron::Slot* inSlot);
    void* slot_fromInt(hadron::Slot* inSlot, int intValue);
} // extern "C"

#endif // SRC_RUNTIME_SLOT_HPP_