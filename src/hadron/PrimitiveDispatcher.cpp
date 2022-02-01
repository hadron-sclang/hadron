#include "hadron/PrimitiveDispatcher.hpp"

#include "hadron/ThreadContext.hpp"

namespace hadron {

Slot dispatchPrimitive(ThreadContext* /* context */, Hash primitiveName) {
//    Slot* sp = context->stackPointer;
    switch (primitiveName) {
//        #include "case/Common/Core/ObjectPrimitiveCases.inl"
    }
    return Slot::makeNil();
}

} // namespace hadron