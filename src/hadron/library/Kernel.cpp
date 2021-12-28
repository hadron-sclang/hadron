#include "schema/Common/Core/Object.hpp"
#include "schema/Common/Core/Kernel.hpp"

namespace hadron {
namespace library {

// static
Slot Interpreter::_CompileExpression(ThreadContext* /* context */, Slot /* _this */, Slot /* string */) {
    return Slot();
}

} // namespace library
} // namespace hadron