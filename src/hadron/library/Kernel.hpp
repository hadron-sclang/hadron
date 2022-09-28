#ifndef SRC_HADRON_LIBRARY_KERNEL_HPP_
#define SRC_HADRON_LIBRARY_KERNEL_HPP_

#include "hadron/Hash.hpp"

#include "hadron/library/Array.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/String.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/library/Thread.hpp"
#include "hadron/schema/Common/Core/KernelSchema.hpp"

#include <cassert>

namespace hadron {
namespace schema {
// Frame has no public members in the class library, so we add some privately here.
struct FramePrivateSchema {
    ~FramePrivateSchema() = delete;
    static constexpr Hash kNameHash = FrameSchema::kNameHash;
    static constexpr Hash kMetaNameHash = FrameSchema::kMetaNameHash;

    library::Schema schema;

    Slot method;
    Slot caller;
    Slot context;
    Slot homeContext;
    Slot arg0;
};
} // namespace schema

namespace library {

class Class;
using ClassArray = TypedArray<Class>;
class FunctionDef;
using FunctionDefArray = TypedArray<FunctionDef>;
class Method;
using MethodArray = TypedArray<Method>;

class Class : public Object<Class, schema::ClassSchema> {
public:
    Class(): Object<Class, schema::ClassSchema>() { }
    explicit Class(schema::ClassSchema* instance): Object<Class, schema::ClassSchema>(instance) { }
    explicit Class(Slot instance): Object<Class, schema::ClassSchema>(instance) { }
    ~Class() { }

    Symbol name(ThreadContext* context) const { return Symbol(context, m_instance->name); }
    void setName(Symbol name) { m_instance->name = name.slot(); }

    Class nextclass() const { return Class(m_instance->nextclass); }
    void setNextclass(Class nextClass) { m_instance->nextclass = nextClass.slot(); }

    Symbol superclass(ThreadContext* context) const { return Symbol(context, m_instance->superclass); }
    void setSuperclass(Symbol name) { m_instance->superclass = name.slot(); }

    ClassArray subclasses() const { return ClassArray(m_instance->subclasses); }
    void setSubclasses(ClassArray a) { m_instance->subclasses = a.slot(); }

    MethodArray methods() const { return MethodArray(m_instance->methods); }
    void setMethods(MethodArray a) { m_instance->methods = a.slot(); }

    SymbolArray instVarNames() const { return SymbolArray(m_instance->instVarNames); }
    void setInstVarNames(SymbolArray a) { m_instance->instVarNames = a.slot(); }

    SymbolArray classVarNames() const { return SymbolArray(m_instance->classVarNames); }
    void setClassVarNames(SymbolArray a) { m_instance->classVarNames = a.slot(); }

    Array iprototype() const { return Array(m_instance->iprototype); }
    void setIprototype(Array a) { m_instance->iprototype = a.slot(); }

    Array cprototype() const { return Array(m_instance->cprototype); }
    void setCprototype(Array a) { m_instance->cprototype = a.slot(); }

    SymbolArray constNames() const { return SymbolArray(m_instance->constNames); }
    void setConstNames(SymbolArray a) { m_instance->constNames = a.slot(); }

    Array constValues() const { return Array(m_instance->constValues); }
    void setConstValues(Array a) { m_instance->constValues = a.slot(); }

    Symbol filenameSymbol(ThreadContext* context) const { return Symbol(context, m_instance->filenameSymbol); }
    void setFilenameSymbol(Symbol filename) { m_instance->filenameSymbol = filename.slot(); }

    int32_t charPos() const { return m_instance->charPos.getInt32(); }
    void setCharPos(int32_t pos) { m_instance->charPos = Slot::makeInt32(pos); }

    int32_t classVarIndex() const { return m_instance->classVarIndex.getInt32(); }
    void setClassVarIndex(int32_t index) { m_instance->classVarIndex = Slot::makeInt32(index); }
};

class Process : public Object<Process, schema::ProcessSchema> {
public:
    Process(): Object<Process, schema::ProcessSchema>() { }
    explicit Process(schema::ProcessSchema* instance): Object<Process, schema::ProcessSchema>(instance) { }
    ~Process() { }

    Thread mainThread() const { return Thread(m_instance->mainThread); }
    void setMainThread(Thread t) { m_instance->mainThread = t.slot(); }

    Thread curThread() const { return Thread(m_instance->curThread); }
    void setCurThread(Thread t) { m_instance->curThread = t.slot(); }
};

template <typename T, typename S> class FunctionDefBase : public Object<T, S> {
public:
    FunctionDefBase(): Object<T, S>() { }
    explicit FunctionDefBase(S* instance): Object<T, S>(instance) { }
    explicit FunctionDefBase(Slot instance): Object<T, S>(instance) { }
    ~FunctionDefBase() { }

    Slot code() const {
        const T& t = static_cast<const T&>(*this);
        return t.m_instance->code;
    }
    void setCode(Slot c) {
        T& t = static_cast<T&>(*this);
        t.m_instance->code = c;
    }

    FunctionDefArray selectors() const {
        const T& t = static_cast<const T&>(*this);
        return FunctionDefArray(t.m_instance->selectors);
    }
    void setSelectors(FunctionDefArray a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->selectors = a.slot();
    }

    // The prototypeFrame in Hadron will be sufficient size to also contain the register spill space.
    Array prototypeFrame() const {
        const T& t = static_cast<const T&>(*this);
        return Array(t.m_instance->prototypeFrame);
    }
    void setPrototypeFrame(Array a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->prototypeFrame = a.slot();
    }

    SymbolArray argNames() const {
        const T& t = static_cast<const T&>(*this);
        return SymbolArray(t.m_instance->argNames);
    }
    void setArgNames(SymbolArray a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->argNames = a.slot();
    }

    SymbolArray varNames() const {
        T& t = static_cast<T&>(*this);
        return SymbolArray(t.m_instance->varNames);
    }
    void setVarNames(SymbolArray a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->varNames = a.slot();
    }
};

class FunctionDef : public FunctionDefBase<FunctionDef, schema::FunctionDefSchema> {
public:
    FunctionDef(): FunctionDefBase<FunctionDef, schema::FunctionDefSchema>() { }
    explicit FunctionDef(schema::FunctionDefSchema* instance):
        FunctionDefBase<FunctionDef, schema::FunctionDefSchema>(instance) { }
    explicit FunctionDef(Slot instance): FunctionDefBase<FunctionDef, schema::FunctionDefSchema>(instance) { }
    ~FunctionDef() { }
};

class Method : public FunctionDefBase<Method, schema::MethodSchema> {
public:
    Method(): FunctionDefBase<Method, schema::MethodSchema>() { }
    explicit Method(schema::MethodSchema* instance): FunctionDefBase<Method, schema::MethodSchema>(instance) { }
    explicit Method(Slot instance): FunctionDefBase<Method, schema::MethodSchema>(instance) { }
    ~Method() { }

    Class ownerClass() const { return Class(m_instance->ownerClass); }
    void setOwnerClass(Class ownerClass) { m_instance->ownerClass = ownerClass.slot(); }

    Symbol name(ThreadContext* context) const { return Symbol(context, m_instance->name); }
    void setName(Symbol name) { m_instance->name = name.slot(); }

    Symbol primitiveName(ThreadContext* context) const { return Symbol(context, m_instance->primitiveName); };
    void setPrimitiveName(Symbol primitiveName) { m_instance->primitiveName = primitiveName.slot(); }

    Symbol filenameSymbol(ThreadContext* context) const { return Symbol(context, m_instance->filenameSymbol); }
    void setFilenameSymbol(Symbol filename) { m_instance->filenameSymbol = filename.slot(); }

    int32_t charPos() const { return m_instance->charPos.getInt32(); }
    void setCharPos(int32_t pos) { m_instance->charPos = Slot::makeInt32(pos); }
};

class Frame : public Object<Frame, schema::FramePrivateSchema> {
public:
    Frame(): Object<Frame, schema::FramePrivateSchema>() { }
    explicit Frame(schema::FramePrivateSchema* instance): Object<Frame, schema::FramePrivateSchema>(instance) { }
    explicit Frame(Slot instance): Object<Frame, schema::FramePrivateSchema>(instance) { }
    ~Frame() { }

    Method method() const { return Method(m_instance->method); }
    void setMethod(Method method) { m_instance->method = method.slot(); }

    Object caller() const { return Object(m_instance->caller); }
    void setCaller(Object caller) { m_instance->caller = caller.slot(); }

    Frame context() const { return Frame(m_instance->context); }
    void setContext(Frame context) { m_instance->context = context.slot(); }

    Frame homeContext() const { return Frame(m_instance->homeContext); }
    void setHomeContext(Frame homeContext) { m_instance->homeContext = homeContext.slot(); }

/*
    const int8_t* ip() const { return m_instance->ip.getRawPointer(); }
    void setIp(const int8_t* ip) { m_instance->ip = Slot::makeRawPointer(ip); }
*/

    Slot arg0() const { return m_instance->arg0; }
    void setArg0(Slot arg) { m_instance->arg0 = arg; }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_KERNEL_HPP_