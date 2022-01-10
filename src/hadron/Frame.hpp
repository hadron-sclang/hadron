#ifndef SRC_HADRON_FRAME_HPP_
#define SRC_HADRON_FRAME_HPP_

#include <list>
#include <memory>

#include "hadron/Slot.hpp"

namespace hadron {

struct Block;

// Represents a stack frame, so can have arguments supplied, is a scope for local variables, has an entrance and exit
// Block.
struct Frame {
    Frame() = default;
    ~Frame() = default;

    // A library::Array with in-order hashes of argument names.
    Slot argumentOrder = nullptr;
    // A library::Array with default values for arguments, if they are literals.
    Slot argumentDefaults = nullptr;

    Frame* parent = nullptr;
    std::list<std::unique_ptr<Block>> blocks;
    std::list<std::unique_ptr<Frame>> subFrames;

    // Values only valid for root frames, subFrame values will be 0.
    size_t numberOfValues = 0;
    int numberOfBlocks = 0;
};

} // namespace hadron

#endif // SRC_HADRON_FRAME_HPP_