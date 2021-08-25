#include "hadron/Slot.hpp"

#include "fmt/format.h"

#include <cassert>

namespace hadron {

bool Slot::operator==(const Slot& s) const {
    if (type != s.type) {
        return false;
    }
    switch(type) {
    case Type::kNil:
        return true;

    case Type::kInteger:
        return value.intValue == s.value.intValue;

    case Type::kFloat:
        return value.floatValue == s.value.floatValue;

    case Type::kBoolean:
        return value.boolValue == s.value.boolValue;

    case Type::kString:
    case Type::kSymbol:
    case Type::kClass:
    case Type::kObject:
    case Type::kArray:
        assert(false);  // TODO
        return false;

    case Type::kAny:
        assert(false); // internal error
        return false;

    case Type::kMachineCodePointer:
        return value.machineCodeAddress == s.value.machineCodeAddress;

    case Type::kStackPointer:
    case Type::kFramePointer:
        return value.slotPointer == s.value.slotPointer;

    case Type::kType:
        return value.typeValue == s.value.typeValue;
    }
}

std::string Slot::asString() {
    if (type == Type::kInteger) {
        return fmt::format("{}", value.intValue);
    }
    return "unknown slot type";
}

} // namespace hadron
