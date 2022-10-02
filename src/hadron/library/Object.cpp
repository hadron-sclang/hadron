#include "hadron/library/Object.hpp"

#include "hadron/Heap.hpp"
#include "hadron/ClassLibrary.hpp"

#include <string_view>

namespace hadron { namespace library {

Slot ObjectBase::_BasicNew(ThreadContext* context, int32_t maxSize) {
    auto name = library::Symbol(context, className());
    // _BasicNew is traditionally called as a class method on Object but expects we will create a new instance, so
    // remove the Meta_ from the front.
    auto nameView = name.view(context);
    assert(nameView.substr(0, 5).compare("Meta_") == 0);
    auto targetName = library::Symbol::fromView(context, nameView.substr(5));

    auto classDef = context->classLibrary->findClassNamed(targetName);
    assert(classDef);

    int32_t sizeInSlots = std::max(maxSize, classDef.instVarNames().size());
    int32_t sizeInBytes = sizeof(library::Schema) + (sizeInSlots * kSlotSize);
    int32_t allocatedSize = 0;
    auto object = reinterpret_cast<library::Schema*>(context->heap->allocateNew(sizeInBytes, allocatedSize));
    object->sizeInBytes = sizeInBytes;
    object->className = targetName.hash();
    object->allocationSize = allocatedSize;
    Slot* contents = reinterpret_cast<Slot*>(object + 1);
    for (int32_t i = 0; i < sizeInSlots; ++i) {
        *contents = Slot::makeNil();
        ++contents;
    }

    return Slot::makePointer(object);
}

} // namespace library
} // namespace hadron