#ifndef SRC_HADRON_LIBRARY_HADRON_FRAME_HPP_
#define SRC_HADRON_LIBRARY_HADRON_FRAME_HPP_

#include "hadron/library/Object.hpp"
#include "hadron/library/HadronHIR.hpp"
#include "hadron/library/HadronScope.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/schema/HLang/HadronFrameSchema.hpp"

namespace hadron {
namespace library {

class Frame : public Object<Frame, schema::HadronFrameSchema> {
public:
    Frame(): Object<Frame, schema::HadronFrameSchema>() {}
    explicit Frame(schema::HadronFrameSchema* instance): Object<Frame, schema::HadronFrameSchema>(instance) {}
    explicit Frame(Slot instance): Object<Frame, schema::HadronFrameSchema>(instance) {}
    ~Frame() {}

    BlockLiteralHIR outerBlockHIR() const { return BlockLiteralHIR(m_instance->outerBlockHIR); }
    void setOuterBlockHIR(BlockLiteralHIR b) { m_instance->outerBlockHIR = b.slot(); }

    Method method() const { return Method(m_instance->method); }
    void setMethod(Method m) { m_instance->method = m.slot(); }

    bool hasVarArgs() const { return m_instance->hasVarArgs.getBool(); }
    void setHasVarArgs(bool b) { m_instance->hasVarArgs = Slot::makeBool(b); }

    SymbolArray variableNames() const { return SymbolArray(m_instance->variableNames); }
    void setVariableNames(SymbolArray a) { m_instance->variableNames = a.slot(); }

    Array prototypeFrame() const { return Array(m_instance->prototypeFrame); }
    void setPrototypeFrame(Array a) { m_instance->prototypeFrame = a.slot(); }

    TypedArray<BlockLiteralHIR> innerBlocks() const { return TypedArray<BlockLiteralHIR>(m_instance->innerBlocks); }
    void setInnerBlocks(TypedArray<BlockLiteralHIR> a) { m_instance->innerBlocks = a.slot(); }

    FunctionDefArray selectors() const { return FunctionDefArray(m_instance->selectors); }
    void setSelectors(FunctionDefArray a) { m_instance->selectors = a.slot(); }

    Scope rootScope() const { return Scope(m_instance->rootScope); }
    void setRootScope(Scope s) { m_instance->rootScope = s.slot(); }

    TypedArray<HIR> values() const { return TypedArray<HIR>(m_instance->values); }
    void setValues(TypedArray<HIR> a) { m_instance->values = a.slot(); }

    int32_t numberOfBlocks() const { return m_instance->numberOfBlocks.getInt32(); }
    void setNumberOfBlocks(int32_t n) { m_instance->numberOfBlocks = Slot::makeInt32(n); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_FRAME_HPP_