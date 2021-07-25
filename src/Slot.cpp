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
