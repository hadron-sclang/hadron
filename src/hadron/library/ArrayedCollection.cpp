#include "schema/Common/Core/Object.hpp"
#include "schema/Common/Collections/Collection.hpp"
#include "schema/Common/Collections/SequenceableCollection.hpp"
#include "schema/Common/Collections/ArrayedCollection.hpp"
#include "schema/Common/Collections/Array.hpp"

#include "hadron/Hash.hpp"
#include "hadron/Heap.hpp"

#include "spdlog/spdlog.h"

namespace hadron {
namespace library {

inline size_t arrayElementSize(Hash className) {
    switch (className.getHash()) {
    case kInt8ArrayHash:
        return 1;
    case kInt16ArrayHash:
        return 2;
    case kInt32ArrayHash:
    case kFloatArrayHash:
        return 4;
    case kDoubleArrayHash:
    case kSymbolArrayHash:
    case kArrayHash:
    default:
        return 8;
    }
}

// Arrays track current number of elements by keeping the _sizeInBytes element in the header up to date. Element zero
// starts after the member variables. In current SC class heirarchy none of the classes derived from ArrayedCollection
// add any instance variables. These static asserts validate that everything is that same size. If these fail we need to
// add code to determine size of the header.
static_assert(sizeof(Int8Array) == sizeof(ObjectHeader));
static_assert(sizeof(Int16Array) == sizeof(ObjectHeader));
static_assert(sizeof(Int32Array) == sizeof(ObjectHeader));
static_assert(sizeof(FloatArray) == sizeof(ObjectHeader));
static_assert(sizeof(DoubleArray) == sizeof(ObjectHeader));
static_assert(sizeof(SymbolArray) == sizeof(ObjectHeader));
static_assert(sizeof(Array) == sizeof(ObjectHeader));

// static
Slot ArrayedCollection::_ArrayAdd(ThreadContext* context, Slot _this, Slot item) {
    // Assumption is this is an instance of ArrayedCollection or a derived class.
    assert(_this.isPointer());
    auto arrayObject = _this.getPointer();
    Hash className = arrayObject->_className;
    auto elementSize = arrayElementSize(className);
    size_t numberOfElements = (arrayObject->_sizeInBytes - sizeof(ObjectHeader)) / elementSize;

    if (arrayObject->_sizeInBytes + elementSize >= context->heap->getAllocationSize(arrayObject)) {
        // Double the size of the array in terms of capacity for number of elements.
        size_t newSize = (numberOfElements * 2 * elementSize) + sizeof(ObjectHeader);
        auto newArray = context->heap->allocateObject(className, newSize);
        if (!newArray) {
            SPDLOG_ERROR("Failed to allocate resize array of {} bytes.", newSize);
            return Slot();
        }
        std::memcpy(newArray, arrayObject, arrayObject->_sizeInBytes);
        arrayObject = newArray;
        _this = Slot(arrayObject);
    }

    ObjectHeader* startOfElements = arrayObject + 1;

    // TODO: type checking in item
    switch (className.getHash()) {
    case kInt8ArrayHash: {
        reinterpret_cast<char*>(startOfElements)[numberOfElements] = item.getChar();
    } break;

    case kInt16ArrayHash: {
        reinterpret_cast<int16_t*>(startOfElements)[numberOfElements] = static_cast<int16_t>(item.getInt32());
    } break;

    case kInt32ArrayHash: {
        reinterpret_cast<int32_t*>(startOfElements)[numberOfElements] = item.getInt32();
    } break;

    case kFloatArrayHash: {
        reinterpret_cast<float*>(startOfElements)[numberOfElements] = static_cast<float>(item.getFloat());
    } break;

    case kDoubleArrayHash:
    case kSymbolArrayHash:
    case kArrayHash:
    default: {
        reinterpret_cast<Slot*>(startOfElements)[numberOfElements] = item;
    } break;
    }

    arrayObject->_sizeInBytes += elementSize;
    return _this;
}

// static
Slot ArrayedCollection::_BasicAt(ThreadContext* /* context */, Slot _this, Slot index) {
    // Assumption is this is an instance of ArrayedCollection or a derived class.
    assert(_this.isPointer());
    auto arrayObject = _this.getPointer();
    Hash className = arrayObject->_className;
    auto elementSize = arrayElementSize(className);
    size_t numberOfElements = (arrayObject->_sizeInBytes - sizeof(ObjectHeader)) / elementSize;
    if (!index.isInt32()) {
        // TODO: non-integral index on ArrayedCollection
        return Slot();
    }
    int32_t intIndex = index.getInt32();
    if (intIndex < 0 || static_cast<size_t>(intIndex) >= numberOfElements) {
        return Slot();
    }
    ObjectHeader* startOfElements = arrayObject + 1;

    switch (className.getHash()) {
    case kInt8ArrayHash:
        return Slot(reinterpret_cast<char*>(startOfElements)[numberOfElements]);

    case kInt16ArrayHash:
        return static_cast<int32_t>(reinterpret_cast<int16_t*>(startOfElements)[numberOfElements]);

    case kInt32ArrayHash:
        return Slot(reinterpret_cast<int32_t*>(startOfElements)[numberOfElements]);

    case kFloatArrayHash:
        return Slot(static_cast<double>(reinterpret_cast<float*>(startOfElements)[numberOfElements]));

    case kDoubleArrayHash:
    case kSymbolArrayHash:
    case kArrayHash:
    default:
        return reinterpret_cast<Slot*>(startOfElements)[numberOfElements];
    }
}

} // namespace library
} // namespace hadron