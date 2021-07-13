#include "hadron/Slot.hpp"

namespace hadron {

} // namespace hadron

extern "C" {
void* slot_fromInt(hadron::Slot* inSlot, int intValue) {
    return new(inSlot) hadron::Slot(intValue);
}
void* slot_Init(hadron::Slot* inSlot) {
    return new(inSlot) hadron::Slot;
}
}