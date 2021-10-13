#include "schema/Common/Collections/Collection.hpp"
#include "schema/Common/Collections/SequenceableCollection.hpp"
#include "schema/Common/Collections/ArrayedCollection.hpp"
#include "schema/Common/Collections/Array.hpp"

#include "hadron/Hash.hpp"

namespace hadron {
namespace library {

inline size_t arrayElementSize(Hash className) {
    switch (className) {
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

}

} // namespace library
} // namespace hadron