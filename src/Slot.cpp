#include "hadron/Slot.hpp"

#include "fmt/format.h"

namespace hadron {

std::string Slot::asString() {
    if (type == Type::kInteger) {
        return fmt::format("{}", intValue);
    }
    return "unknown slot type";
}


} // namespace hadron

extern "C" {
void* slot_fromInt(hadron::Slot* inSlot, int intValue) {
    return new(inSlot) hadron::Slot(intValue);
}
void* slot_Init(hadron::Slot* inSlot) {
    return new(inSlot) hadron::Slot;
}
}