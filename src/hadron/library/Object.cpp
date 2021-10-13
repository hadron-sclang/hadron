#include "schema/Common/Core/Object.hpp"

namespace hadron {
namespace library {

// static
Slot Object::_BasicNew(ThreadContext* /* context */, Slot /* _this */, Slot /* maxSize */) {
    return Slot();
}

// static
Slot Object::_BasicNewCopyArgsToInstVars(ThreadContext* /* context */, Slot /* _this */) {
    return Slot();
}

} // namespace library
} // namespace hadron