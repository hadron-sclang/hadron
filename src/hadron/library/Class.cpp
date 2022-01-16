#include "schema/Common/Core/Object.hpp"
#include "schema/Common/Core/Kernel.hpp"

namespace hadron {
namespace library {

// static
Slot Class::_AllClasses(ThreadContext* /* context */, Slot /* _this */) {
    return Slot::makeNil();
}

// static
Slot Class::_DumpClassSubtree(ThreadContext* /* context */, Slot /* _this */) {
    return Slot::makeNil();
}

} // namespace library
} // namespace hadron