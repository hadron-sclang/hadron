#ifndef SRC_RUNTIME_SLOT_HPP_
#define SRC_RUNTIME_SLOT_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Type.hpp"

#include <cstddef>
#include <string>

namespace hadron {

struct Slot {
public:
    Slot(): type(kNil), value(nullptr) {}
    Slot(int32_t intValue): type(Type::kInteger), value(intValue) {}
    Slot(uint8_t* machineCodeAddress): type(Type::kMachineCodePointer), value(machineCodeAddress) {}
    Slot(Type pointerType, Slot* pointer): type(pointerType), value(pointer) {}
    ~Slot() = default;

    // Placement new
    void* operator new(size_t, Slot* address) { return address; }

    std::string asString();

    Type type;
    uint32_t padding;
    union Value {
        Value(): machineCodeAddress(nullptr) {}
        Value(int32_t v): intValue(v) {}
        Value(uint8_t* a): machineCodeAddress(a) {}
        Value(Slot* p): slotPointer(p) {}
        Value(nullptr_t): slotPointer(nullptr) {}

        int32_t intValue;
        double doubleValue;
        uint8_t* machineCodeAddress;
        Slot* slotPointer;
        Hash symbolHash;
    };
    Value value;
};

static_assert(sizeof(Slot) == 16);

} // namespace hadron

#endif // SRC_RUNTIME_SLOT_HPP_