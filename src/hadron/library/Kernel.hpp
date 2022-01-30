#ifndef SRC_HADRON_LIBRARY_CLASS_HPP_
#define SRC_HADRON_LIBRARY_CLASS_HPP_

#include "hadron/Hash.hpp"

#include "hadron/library/Object.hpp"
#include "hadron/library/Array.hpp"

#include "schema/Common/Core/KernelSchema.hpp"

namespace hadron {
namespace library {

class Class : public Object<Class, schema::ClassSchema> {
public:
    Class() = delete;
    explicit Class(schema::ClassSchema* instance);
    ~Class() {}

    Hash name() const { return m_instance->name.getHash(); }
    void setName(Hash n) { m_instance->name = Slot::makeHash(n); }
    Class nextclass() const {
        return Class(reinterpret_cast<schema::ClassSchema*>(m_instance->nextclass.getPointer()));
    }
    void setNextclass(Class c) { m_instance->nextclass = Slot::makePointer(c.instance()); }
};

template<typename T, typename S>
class FunctionDefBase : public Object<T, S> {
public:
    explicit FunctionDefBase(S* instance): Object<T, S>(instance) {
        instance->raw1 = Slot::makeNil();
        instance->raw2 = Slot::makeNil();
        instance->code = Slot::makeNil();
        instance->selectors = Slot::makeNil();
        instance->constants = Slot::makeNil();
        instance->prototypeFrame = Slot::makeNil();
        instance->context = Slot::makeNil();
        instance->argNames = Slot::makeNil();
        instance->varNames = Slot::makeNil();
        instance->sourceCode = Slot::makeNil();
    }

    SymbolArray argNames() const {
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
    FunctionDef() = delete;
    explicit FunctionDef(schema::FunctionDefSchema* instance):
        FunctionDefBase<FunctionDef, schema::FunctionDefSchema>(instance) {}
    ~FunctionDef() {}
};

class Method : public FunctionDefBase<Method, schema::MethodSchema> {
public:
    Method() = delete;
    explicit Method(schema::MethodSchema* instance);
    ~Method() {}
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_CLASS_HPP_