#ifndef SRC_HADRON_FRAME_HPP_
#define SRC_HADRON_FRAME_HPP_

#include <memory>

#include "hadron/Slot.hpp"

namespace hadron {

struct Scope;

// Represents a stack frame, so can have arguments supplied and can be called so has an entry, return value, and exit.
struct Frame {
    Frame() = default;
    ~Frame() = default;

    // A library::Array with in-order hashes of argument names.
    Slot argumentOrder = Slot::makeNil();
    // A library::Array with default values for arguments, if they are literals.
    Slot argumentDefaults = Slot::makeNil();

    // If true, the last argument named in the list is a variable argument array.
    bool hasVarArgs = false;

    std::unique_ptr<Scope> rootScope;

    size_t numberOfValues = 0; // actual number of values used could be less than this due to optimization
    int numberOfBlocks = 0;
};

} // namespace hadron

#endif // SRC_HADRON_FRAME_HPP_