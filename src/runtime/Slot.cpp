#include "Slot.hpp"

namespace hadron {


} // namespace hadron

extern "C" int rt_Slot_asInt(const hadron::Slot* slot) {
    return slot->asInt();
}