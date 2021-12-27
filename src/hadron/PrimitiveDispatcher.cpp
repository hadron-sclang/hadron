#include "hadron/PrimitiveDispatcher.hpp"

#include "hadron/ThreadContext.hpp"
#include "schema/Common/Core/Object.hpp"

namespace hadron {

Slot dispatchPrimitive(ThreadContext* context, Hash primitiveName) {
    Slot* sp = context->stackPointer;
    switch (primitiveName.getHash()) {
        #include "case/Common/Core/ObjectPrimitiveCases.inl"
    }
    return Slot();
}

} // namespace hadron