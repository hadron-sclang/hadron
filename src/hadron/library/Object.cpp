#include "schema/Common/Core/Object.hpp"

#include <cassert>

namespace hadron {
namespace library {

// static
Slot Object::_BasicNew(ThreadContext* /* context */, Slot /* _this */, Slot /* maxSize */) {
    assert(false); // WRITEME
    // |maxSize| is for array-type initializations that need to provide larger capacity than needed for their internal
    // storage. It's an integer type, but what are the units?
    // As _BasicNew is called from a class method |_this| should be an instance of Class, and _BasicNew can initialize
    // the variables from Class::iprototype.
    return Slot::makeNil();
}

// static
Slot Object::_BasicNewCopyArgsToInstVars(ThreadContext* /* context */, Slot /* _this */) {
    assert(false);
    return Slot::makeNil();
}

// static
Slot Object::_ObjectDeepCopy(ThreadContext* /* context */, Slot /* _this */) {
    assert(false);
    return Slot::makeNil();
}

} // namespace library
} // namespace hadron