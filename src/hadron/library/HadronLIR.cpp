#include "hadron/library/HadronLIR.hpp"

namespace hadron {
namespace library {

bool LIR::producesValue() const {
    switch (className()) {
    case AssignLIR::nameHash():
        return true;

    case LabelLIR::nameHash():
        return false;
    }

    // Likely missing a type from the switch statement above.
    assert(false);
    return false;
}

} // namespace library
} // namespace hadron