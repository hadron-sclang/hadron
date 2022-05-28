#ifndef SRC_HADRON_MATERIALIZER_HPP_
#define SRC_HADRON_MATERIALIZER_HPP_

#include "library/ArrayedCollection.hpp"
#include "library/HadronCFG.hpp"

namespace hadron {

struct ThreadContext;

// Utility class to take a Frame of HIR code and produce an Int8Array of the finalized bytecode.
class Materializer {
public:
    // May recursively materialize subframes first.
    static library::Int8Array materialize(ThreadContext* context, library::CFGFrame frame);
};

} // namespace hadron

#endif // SRC_HADRON_MATERIALIZER_HPP_