#ifndef SRC_HADRON_MATERIALIZER_HPP_
#define SRC_HADRON_MATERIALIZER_HPP_

#include "library/ArrayedCollection.hpp"

namespace hadron {

struct ThreadContext;
struct Frame;

// Utility class to take a Frame of HIR code and produce an Int8Array of the finalized bytecode.
class Materializer {
public:
    // May recursively materialize subframes first.
    static library::Int8Array materialize(ThreadContext* context, Frame* frame);
};

} // namespace hadron

#endif // SRC_HADRON_MATERIALIZER_HPP_