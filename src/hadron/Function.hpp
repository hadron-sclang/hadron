#ifndef SRC_COMPILER_INCLUDE_HADRON_FUNCTION_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_FUNCTION_HPP_

#include "hadron/Hash.hpp"
#include "hadron/JITMemoryArena.hpp"
#include "hadron/Slot.hpp"

#include <memory>

namespace hadron {

class LighteningJIT;
struct ThreadContext;

// Represents a unit of executable SuperCollider code.
struct Function {
    Function() = default;
    ~Function() = default;

    int numberOfArgs;
    // Argument names in order.
    std::unique_ptr<Hash[]> argumentNames;
    std::unique_ptr<Slot[]> defaultValues;

    const uint8_t* machineCode = nullptr;
    JITMemoryArena::MCodePtr machineCodeOwned;
};

}

#endif // SRC_COMPILER_INCLUDE_HADRON_FUNCTION_HPP_