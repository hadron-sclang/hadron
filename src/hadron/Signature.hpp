#ifndef SRC_HADRON_SIGNATURE_HPP_
#define SRC_HADRON_SIGNATURE_HPP_

#include "hadron/library/Kernel.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/Slot.hpp"

#include <vector>

namespace hadron {

struct Signature {
    Signature() = default;
    ~Signature() = default;

    library::Symbol selector;
    std::vector<TypeFlags> argumentTypes;

    // Only valid for those arguments that have kObject in their type flags. For those that do, represents the class if
    // known. If nil it means the class is unknown.
    std::vector<library::Symbol> argumentClassNames;
};

} // namespace hadron

#endif