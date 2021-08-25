#ifndef SRC_RUNTIME_SLOT_HPP_
#define SRC_RUNTIME_SLOT_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Type.hpp"

#include <cstddef>
#include <string>

namespace hadron {

struct Slot {
public:
    union Value {
        Value(): machineCodeAddress(nullptr) {}
        Value(int32_t v): intValue(v) {}
        Value(double v): floatValue(v) {}
        Value(bool v): boolValue(v) {}
        Value(uint8_t* a): machineCodeAddress(a) {}
        Value(Slot* p): slotPointer(p) {}
        Value(nullptr_t): slotPointer(nullptr) {}
        Value(Hash h): symbolHash(h) {}
        Value(Type t): typeValue(t) {}

        int32_t intValue;
        double floatValue;
        bool boolValue;
        uint8_t* machineCodeAddress;
        Slot* slotPointer;
        Hash symbolHash;
#       ifdef HADRON_64_BIT
        int64_t registerSpill;
#       else
        int32_t registerSpill;
#       endif
        Type typeValue;
    };

    Slot(): type(kNil), size(0), value(nullptr) {}
    Slot(Type t, Value v): type(t), value(v) {}
    ~Slot() = default;

    // TODO: deprecate these other ctors
    Slot(int32_t intValue): type(Type::kInteger), size(0), value(intValue) {}
    Slot(double floatValue): type(Type::kFloat), size(0), value(floatValue) {}
    Slot(bool boolValue): type(Type::kBoolean), size(0), value(boolValue) {}
    // For strings and symbols in the parser/lexer that need to be copied and allocated. TODO: add copy
    Slot(Type stringType): type(stringType), size(0), value(nullptr) {}
    Slot(uint8_t* machineCodeAddress): type(Type::kMachineCodePointer), size(0), value(machineCodeAddress) {}
    Slot(Type pointerType, Slot* pointer): type(pointerType), size(0), value(pointer) {}

    // Placement new
    void* operator new(size_t, Slot* address) { return address; }

    // Comparisons
    bool operator==(const Slot& s) const;

    std::string asString();

    Type type;
    int32_t size;
    Value value;

    // Offset in bytes given a slotNumber (which can be negative)
    static inline int slotTypeOffset(int slotNumber) {
        return (slotNumber * sizeof(Slot)) + static_cast<int>(offsetof(Slot, type));
    }
    static inline int slotValueOffset(int slotNumber) {
        return (slotNumber * sizeof(Slot)) + static_cast<int>(offsetof(Slot, value));
    }
};

static_assert(sizeof(Slot) == 16);

} // namespace hadron

#endif // SRC_RUNTIME_SLOT_HPP_