#include "hadron/library/HadronHIR.hpp"

namespace hadron {
namespace library {

HIRId HIR::proposeId(HIRId proposedId) {
    switch (className()) {
    case BlockLiteralHIR::nameHash():
        setId(proposedId);
        return proposedId;
    }

    // Likely missing a type from the switch statement above.
    assert(false);
    return HIRId();
}

} // namespace library
} // namespace hadron