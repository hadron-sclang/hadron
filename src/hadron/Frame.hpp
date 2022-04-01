#ifndef SRC_HADRON_FRAME_HPP_
#define SRC_HADRON_FRAME_HPP_

#include "hadron/hir/HIR.hpp"
#include "hadron/library/Array.hpp"
#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/library/Kernel.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hadron {

namespace hir {
struct BlockLiteralHIR;
}

class Block;
struct Scope;

// Represents a stack frame, so can have arguments supplied and can be called so has an entry, return value, and exit.
struct Frame {
    Frame() = delete;
    Frame(hir::BlockLiteralHIR* outerBlock, library::Method meth);
    ~Frame() = default;

    // Replaces pairs (key, value). May cause other replacements, which are handled sequentially. Destructively modifies
    // the |replacements| map. All pointers to HIR must be owned by blocks within this frame.
    void replaceValues(std::unordered_map<hir::HIR*, hir::HIR*>& replacements);

    // Function literals can capture values from outside frames, so we include a pointer to the InlineBlockHIR in the
    // containing frame to support search of those frames for those values.
    hir::BlockLiteralHIR* outerBlockHIR;

    library::Method method;

    // If true, the last argument named in the list is a variable argument array.
    bool hasVarArgs;

    // Flattened variable name array.
    library::SymbolArray variableNames;
    // Initial values, for concatenation onto the prototypeFrame.
    library::Array prototypeFrame;

    std::unique_ptr<Scope> rootScope;

    // Map of value IDs as index to HIR objects. During optimization HIR can change, for example simplifying MessageHIR
    // to a constant, so we identify values by their NVID and maintain a single frame-wide map here of the authoritative
    // map between IDs and HIR.
    std::vector<hir::HIR*> values;

    // Counter used as a serial number to uniquely identify blocks.
    int32_t numberOfBlocks;

    // To avoid creation of duplicate constants we track all constant values in a map.
    std::unordered_map<Slot, hir::ID> constantValues;

    // For quickly determining if a given ID is a constant.
    std::unordered_set<hir::ID> constantIds;
};

} // namespace hadron

#endif // SRC_HADRON_FRAME_HPP_