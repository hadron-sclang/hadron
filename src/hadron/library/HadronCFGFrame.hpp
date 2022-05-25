#ifndef SRC_HADRON_LIBRARY_HADRON_CFGFRAME_HPP_
#define SRC_HADRON_LIBRARY_HADRON_CFGFRAME_HPP_

#include "hadron/library/HadronCFGScope.hpp"
#include "hadron/library/HadronHIR.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/schema/HLang/HadronCFGFrameSchema.hpp"

namespace hadron {
namespace library {

class CFGFrame : public Object<CFGFrame, schema::HadronCFGFrameSchema> {
public:
    CFGFrame(): Object<CFGFrame, schema::HadronCFGFrameSchema>() {}
    explicit CFGFrame(schema::HadronCFGFrameSchema* instance):
            Object<CFGFrame, schema::HadronCFGFrameSchema>(instance) {}
    explicit CFGFrame(Slot instance): Object<CFGFrame, schema::HadronCFGFrameSchema>(instance) {}
    ~CFGFrame() {}

    static CFGFrame makeCFGFrame(ThreadContext* context, BlockLiteralHIR outerBlock, Method method) {
        auto frame = CFGFrame::alloc(context);
        frame.initToNil();
        frame.setOuterBlockHIR(outerBlock);
        frame.setMethod(method);
        frame.setHasVarArgs(false);
        frame.setRootScope(Scope::makeRootScope(context, frame));
        frame.setNumberOfBlocks(0);
        return frame;
    }

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

#endif // SRC_HADRON_LIBRARY_HADRON_CFGFRAME_HPP_