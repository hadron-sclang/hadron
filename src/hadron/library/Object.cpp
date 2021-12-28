#include "schema/Common/Core/Object.hpp"

namespace hadron {
namespace library {

// static
Slot Object::_BasicNew(ThreadContext* /* context */, Slot /* _this */, Slot /* maxSize */) {
    // |maxSize| is for array-type initializations that need to provide larger capacity than needed for their internal
    // storage. It's an integer type in units of Slots.
    // As _BasicNew is called from a class method |_this| should be an instance of Class, and _BasicNew can initialize
    // the variables from Class::iprototype.
    return Slot();
}

// static
Slot Object::_BasicNewCopyArgsToInstVars(ThreadContext* /* context */, Slot /* _this */) {
    return Slot();
}

} // namespace library
} // namespace hadron