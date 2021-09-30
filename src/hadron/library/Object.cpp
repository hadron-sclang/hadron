#include "schema/Common/Core/Object.hpp"

namespace hadron {

Slot Object::_BasicNew(ThreadContext* /* context */, Slot /* maxSize */) {
    return Slot();
}

} // namespace hadron