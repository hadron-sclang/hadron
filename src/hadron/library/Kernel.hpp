#ifndef SRC_HADRON_LIBRARY_CLASS_HPP_
#define SRC_HADRON_LIBRARY_CLASS_HPP_

#include "hadron/Hash.hpp"

#include "hadron/library/Array.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/schema/Common/Core/KernelSchema.hpp"

namespace hadron {
namespace library {

class Class;
using ClassArray = TypedArray<Class>;
class Method;
using MethodArray = TypedArray<Method>;

class Class : public Object<Class, schema::ClassSchema> {
public:
    Class(): Object<Class, schema::ClassSchema>() {}
    explicit Class(schema::ClassSchema* instance): Object<Class, schema::ClassSchema>(instance) {}
    explicit Class(Slot instance): Object<Class, schema::ClassSchema>(instance) {}
    ~Class() {}

    Symbol name(ThreadContext* context) const { return Symbol::fromHash(context, m_instance->name.getHash()); }
    void setName(Symbol name) { m_instance->name = name.slot(); }

    Class nextclass() const { return Class(m_instance->nextclass); }
    void setNextclass(Class nextClass) { m_instance->nextclass = nextClass.slot(); }

    Symbol superclass(ThreadContext* context) const {
        return Symbol::fromHash(context, m_instance->superclass.getHash());
    }
    void setSuperclass(Symbol name) { m_instance->superclass = name.slot(); }

    const ClassArray subclasses() const { return ClassArray(m_instance->subclasses); }
    void setSubclasses(ClassArray a) { m_instance->subclasses = a.slot(); }

    // Access to arrays comes with the complication that a new wrapper is returned so things that modify the underlying
    // instance variable (e.g. addTyped()) won't reflect those changed here in the Class instance.
    const MethodArray methods() const { return MethodArray(m_instance->methods); }
    void setMethods(MethodArray a) { m_instance->methods = a.slot(); }

    const SymbolArray instVarNames() const { return SymbolArray(m_instance->instVarNames); }
    void setInstVarNames(SymbolArray a) { m_instance->instVarNames = a.slot(); }

    const SymbolArray classVarNames() const { return SymbolArray(m_instance->classVarNames); }
    void setClassVarNames(SymbolArray a) { m_instance->classVarNames = a.slot(); }

    const Array iprototype() const { return Array(m_instance->iprototype); }
    void setIprototype(Array a) { m_instance->iprototype = a.slot(); }

    const Array cprototype() const { return Array(m_instance->cprototype); }
    void setCprototype(Array a) { m_instance->cprototype = a.slot(); }

    const SymbolArray constNames() const { return SymbolArray(m_instance->constNames); }
    void setConstNames(SymbolArray a) { m_instance->constNames = a.slot(); }

    const Array constValues() const { return Array(m_instance->constValues); }
    void setConstValues(Array a) { m_instance->constValues = a.slot(); }

    Symbol filenameSymbol(ThreadContext* context) const {
        return Symbol::fromHash(context, m_instance->filenameSymbol.getHash());
    }
    void setFilenameSymbol(Symbol filename) { m_instance->filenameSymbol = filename.slot(); }

    int32_t charPos() const { return m_instance->charPos.getInt32(); }
    void setCharPos(int32_t pos) { m_instance->charPos = Slot::makeInt32(pos); }
};

class Process : public Object<Process, schema::ProcessSchema> {
public:
    Process(): Object<Process, schema::ProcessSchema>() {}
    explicit Process(schema::ProcessSchema* instance): Object<Process, schema::ProcessSchema>(instance) {}
    ~Process() {}
};

template<typename T, typename S>
class FunctionDefBase : public Object<T, S> {
public:
    FunctionDefBase(): Object<T, S>() {}

    explicit FunctionDefBase(S* instance): Object<T, S>(instance) {}
    explicit FunctionDefBase(Slot instance): Object<T, S>(instance) {}

    const Int8Array code() const {
        T& t = static_cast<T&>(*this);
        return Int8Array(t.m_instance->code);
    }
    void setCode(Int8Array c) {
        T& t = static_cast<T&>(*this);
        t.m_instance->code = c.slot();
    }

    const Array prototypeFrame() const {
        T& t = static_cast<T&>(*this);
        return Array(t.m_instance->prototypeFrame);
    }
    void setPrototypeFrame(Array a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->prototypeFrame = a.slot();
    }

    const SymbolArray argNames() const {
        T& t = static_cast<T&>(*this);
        return SymbolArray(t.m_instance->argNames);
    }
    void setArgNames(SymbolArray a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->argNames = a.slot();
    }
};

class FunctionDef : public FunctionDefBase<FunctionDef, schema::FunctionDefSchema> {
public:
    FunctionDef(): FunctionDefBase<FunctionDef, schema::FunctionDefSchema>() {}
    explicit FunctionDef(schema::FunctionDefSchema* instance):
        FunctionDefBase<FunctionDef, schema::FunctionDefSchema>(instance) {}
    explicit FunctionDef(Slot instance):
        FunctionDefBase<FunctionDef, schema::FunctionDefSchema>(instance) {}
    ~FunctionDef() {}
};

class Method : public FunctionDefBase<Method, schema::MethodSchema> {
public:
    Method(): FunctionDefBase<Method, schema::MethodSchema>() {}
    explicit Method(schema::MethodSchema* instance):
        FunctionDefBase<Method, schema::MethodSchema>(instance) {}
    explicit Method(Slot instance):
        FunctionDefBase<Method, schema::MethodSchema>(instance) {}
    ~Method() {}

    Class ownerClass() const { return Class(m_instance->ownerClass); }
    void setOwnerClass(Class ownerClass) { m_instance->ownerClass = ownerClass.slot(); }

    Symbol name(ThreadContext* context) const { return Symbol::fromHash(context, m_instance->name.getHash()); }
    void setName(Symbol name) { m_instance->name = name.slot(); }

    Symbol primitiveName(ThreadContext* context) const {
        return Symbol::fromHash(context, m_instance->primitiveName.getHash());
    };
    void setPrimitiveName(Symbol primitiveName) { m_instance->primitiveName = primitiveName.slot(); }

    Symbol filenameSymbol(ThreadContext* context) const {
        return Symbol::fromHash(context, m_instance->filenameSymbol.getHash());
    }
    void setFilenameSymbol(Symbol filename) { m_instance->filenameSymbol = filename.slot(); }

    int32_t charPos() const { return m_instance->charPos.getInt32(); }
    void setCharPos(int32_t pos) { m_instance->charPos = Slot::makeInt32(pos); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_CLASS_HPP_