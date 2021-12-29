#ifndef SRC_HADRON_FRAME_HPP_
#define SRC_HADRON_FRAME_HPP_

namespace hadron {

// Represents a stack frame, so can have arguments supplied, is a scope for local variables, has an entrance and exit
// Block.
struct Frame {
    Frame(): parent(nullptr) {}
    ~Frame() = default;

    // In-order hashes of argument names.
    std::vector<Hash> argumentOrder;

    Frame* parent;
    std::list<std::unique_ptr<Block>> blocks;
    std::list<std::unique_ptr<Frame>> subFrames;

    // Values only valid for root frames, subFrame values will be 0.
    size_t numberOfValues = 0;
    int numberOfBlocks = 0;
};


} // namespace hadron

#endif // SRC_HADRON_FRAME_HPP_