#ifndef SRC_HADRON_FRAME_HPP_
#define SRC_HADRON_FRAME_HPP_

#include "hadron/hir/HIR.hpp"
#include "hadron/library/Array.hpp"
#include "hadron/library/ArrayedCollection.hpp"

#include <memory>
#include <vector>

namespace hadron {

struct Scope;

// Represents a stack frame, so can have arguments supplied and can be called so has an entry, return value, and exit.
struct Frame {
    Frame() = default;
    ~Frame() = default;

    // A library::Array with in-order hashes of argument names.
    library::SymbolArray argumentOrder;
    // A library::Array with default values for arguments, if they are literals.
    library::Array argumentDefaults;

    // If true, the last argument named in the list is a variable argument array.
    bool hasVarArgs = false;

    std::unique_ptr<Scope> rootScope;

    // Map of value IDs as index to HIR objects. During optimization HIR can change, for example simplifying MessageHIR
    // to a constant, so we identify values by their NVID and maintain a single frame-wide map here of the authoritative
    // map between IDs and HIR.
    std::vector<hir::HIR*> values;

    // Counters used as serial numbers to uniquely number values and blocks.
    int32_t numberOfBlocks = 0;
};

} // namespace hadron

#endif // SRC_HADRON_FRAME_HPP_