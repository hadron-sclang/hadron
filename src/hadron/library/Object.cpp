#include "schema/Common/Core/Object.hpp"

namespace hadron {

Slot Object::_BasicNew(ThreadContext* /* context */, Slot /* maxSize */) {
    return Slot();
}

Slot Object::_BasicNewCopyArgsToInstVars(ThreadContext* /* context */) {
    return Slot();
}

} // namespace hadron