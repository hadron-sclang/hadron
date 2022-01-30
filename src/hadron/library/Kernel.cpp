#include "hadron/library/Kernel.hpp"

namespace hadron {
namespace library {

Class::Class(schema::ClassSchema* instance):
    Object<Class, schema::ClassSchema>(instance) {
    m_instance->name = Slot::makeNil();
    m_instance->nextclass = Slot::makeNil();
    m_instance->superclass = Slot::makeNil();
    m_instance->subclasses = Slot::makeNil();
    m_instance->methods = Slot::makeNil();
    m_instance->instVarNames = Slot::makeNil();
    m_instance->classVarNames = Slot::makeNil();
    m_instance->iprototype = Slot::makeNil();
    m_instance->cprototype = Slot::makeNil();
    m_instance->constNames = Slot::makeNil();
    m_instance->instanceFormat = Slot::makeNil();
    m_instance->instanceFlags = Slot::makeNil();
    m_instance->classIndex = Slot::makeNil();
    m_instance->classFlags = Slot::makeNil();
    m_instance->maxSubclassIndex = Slot::makeNil();
    m_instance->filenameSymbol = Slot::makeNil();
    m_instance->charPos = Slot::makeNil();
    m_instance->classVarIndex = Slot::makeNil();
}

Method::Method(schema::MethodSchema* instance):
    FunctionDefBase<Method, schema::MethodSchema>(instance) {
    m_instance->ownerClass = Slot::makeNil();
    m_instance->name = Slot::makeNil();
    m_instance->primitiveName = Slot::makeNil();
    m_instance->filenameSymbol = Slot::makeNil();
    m_instance->charPos = Slot::makeNil();
}

} // namespace library
} // namespace hadron