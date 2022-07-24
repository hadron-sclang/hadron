#ifndef SRC_HADRON_LIBRARY_HADRON_CFG_HPP_
#define SRC_HADRON_LIBRARY_HADRON_CFG_HPP_

#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/library/Dictionary.hpp"
#include "hadron/library/HadronHIR.hpp"
#include "hadron/library/Integer.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/library/Object.hpp"

#include "hadron/schema/HLang/HadronCFGSchema.hpp"

namespace hadron {
namespace library {

/*
 * Dependency graph of the various CFG objects and HIR:
 * +----------+
 * | CFGFrame |
 * +----------+
 *      |
 *      |
 *      V
 * +----------+
 * | CFGScope |
 * +----------+
 *      |
 *      |
 *      V
 * +----------+
 * | CFGBlock |
 * +----------+
 *      |
 *      V
 *   +-----+
 *   | HIR |
 *   +-----+
 */

class CFGFrame;
class CFGBlock : public Object<CFGBlock, schema::HadronCFGBlockSchema> {
public:
    CFGBlock(): Object<CFGBlock, schema::HadronCFGBlockSchema>() {}
    explicit CFGBlock(schema::HadronCFGBlockSchema* instance):
            Object<CFGBlock, schema::HadronCFGBlockSchema>(instance) {}
    explicit CFGBlock(Slot instance): Object<CFGBlock, schema::HadronCFGBlockSchema>(instance) {}
    ~CFGBlock() {}

    static CFGBlock makeCFGBlock(ThreadContext* context, int32_t blockId) {
        auto block = CFGBlock::alloc(context);
        block.initToNil();
        block.setId(blockId);
        block.setHasMethodReturn(false);
        block.setConstantValues(TypedIdentDict<Slot, HIRId>::makeTypedIdentDict(context));
        block.setConstantIds(TypedIdentSet<Integer>::makeTypedIdentSet(context));
        return block;
    }

    // Adds hir to |statements| or |exitStatements|, returns assigned id.
    HIRId append(ThreadContext* context, CFGFrame frame, HIR hir);

    int32_t id() const { return m_instance->id.getInt32(); }
    void setId(int32_t i) { m_instance->id = Slot::makeInt32(i); }

    Int32Array predecessors() const { return Int32Array(m_instance->predecessors); }
    void setPredecessors(Int32Array a) { m_instance->predecessors = a.slot(); }

    Int32Array successors() const { return Int32Array(m_instance->successors); }
    void setSuccessors(Int32Array a) { m_instance->successors = a.slot(); }

    TypedArray<PhiHIR> phis() const { return TypedArray<PhiHIR>(m_instance->phis); }
    void setPhis(TypedArray<PhiHIR> a) { m_instance->phis = a.slot(); }

    TypedArray<HIR> statements() const { return TypedArray<HIR>(m_instance->statements); }
    void setStatements(TypedArray<HIR> a) { m_instance->statements = a.slot(); }

    // TODO: can refactor BranchIfTrueHIR to take two arguments, then there's only ever 1 exit statement from a block
    TypedArray<HIR> exitStatements() const { return TypedArray<HIR>(m_instance->exitStatements); }
    void setExitStatements(TypedArray<HIR> a) { m_instance->exitStatements = a.slot(); }

    bool hasMethodReturn() const { return m_instance->hasMethodReturn.getBool(); }
    void setHasMethodReturn(bool b) { m_instance->hasMethodReturn = Slot::makeBool(b); }

    HIRId finalValue() const { return HIRId(m_instance->finalValue); }
    void setFinalValue(HIRId id) { m_instance->finalValue = id.slot(); }

    TypedIdentDict<Slot, HIRId> constantValues() const {
        return TypedIdentDict<Slot, HIRId>(m_instance->constantValues);
    }
    void setConstantValues(TypedIdentDict<Slot, HIRId> tid) { m_instance->constantValues = tid.slot(); }

    HIRId nilConstantValue() const { return HIRId(m_instance->nilConstantValue); }
    void setNilConstantValue(HIRId i) { m_instance->nilConstantValue = i.slot(); }

    TypedIdentSet<Integer> constantIds() const { return TypedIdentSet<Integer>(m_instance->constantIds); }
    void setConstantIds(TypedIdentSet<Integer> tis) { m_instance->constantIds = tis.slot(); }

    Integer loopReturnPredIndex() const { return Integer(m_instance->loopReturnPredIndex); }
    void setLoopReturnPredIndex(Integer i) { m_instance->loopReturnPredIndex = i.slot(); }
};

class CFGScope : public Object<CFGScope, schema::HadronCFGScopeSchema> {
public:
    CFGScope(): Object<CFGScope, schema::HadronCFGScopeSchema>() {}
    explicit CFGScope(schema::HadronCFGScopeSchema* instance):
            Object<CFGScope, schema::HadronCFGScopeSchema>(instance) {}
    explicit CFGScope(Slot instance): Object<CFGScope, schema::HadronCFGScopeSchema>(instance) {}
    ~CFGScope() {}

    static CFGScope makeCFGScope(ThreadContext* context) {
        auto scope = CFGScope::alloc(context);
        scope.initToNil();
        scope.setFrameIndex(0);
        scope.setValueIndices(TypedIdentDict<Symbol, Integer>::makeTypedIdentDict(context));
        return scope;
    }

    TypedArray<CFGBlock> blocks() const { return TypedArray<CFGBlock>(m_instance->blocks); }
    void setBlocks(TypedArray<CFGBlock> a) { m_instance->blocks = a.slot(); }

    TypedArray<CFGScope> subScopes() const { return TypedArray<CFGScope>(m_instance->subScopes); }
    void setSubScopes(TypedArray<CFGScope> a) { m_instance->subScopes = a.slot(); }

    int32_t frameIndex() const { return m_instance->frameIndex.getInt32(); }
    void setFrameIndex(int32_t i) { m_instance->frameIndex = Slot::makeInt32(i); }

    TypedIdentDict<Symbol, Integer> valueIndices() const {
        return TypedIdentDict<Symbol, Integer>(m_instance->valueIndices); }
    void setValueIndices(TypedIdentDict<Symbol, Integer> tid) { m_instance->valueIndices = tid.slot(); }
};

class CFGFrame : public Object<CFGFrame, schema::HadronCFGFrameSchema> {
public:
    CFGFrame(): Object<CFGFrame, schema::HadronCFGFrameSchema>() {}
    explicit CFGFrame(schema::HadronCFGFrameSchema* instance):
            Object<CFGFrame, schema::HadronCFGFrameSchema>(instance) {}
    explicit CFGFrame(Slot instance): Object<CFGFrame, schema::HadronCFGFrameSchema>(instance) {}
    ~CFGFrame() {}

    static CFGFrame makeCFGFrame(ThreadContext* context) {
        auto frame = CFGFrame::alloc(context);
        frame.initToNil();
        frame.setHasVarArgs(false);
        frame.setRootScope(CFGScope::makeCFGScope(context));
        frame.setNumberOfBlocks(0);
        return frame;
    }

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

    CFGScope rootScope() const { return CFGScope(m_instance->rootScope); }
    void setRootScope(CFGScope s) { m_instance->rootScope = s.slot(); }

    TypedArray<HIR> values() const { return TypedArray<HIR>(m_instance->values); }
    void setValues(TypedArray<HIR> a) { m_instance->values = a.slot(); }

    int32_t numberOfBlocks() const { return m_instance->numberOfBlocks.getInt32(); }
    void setNumberOfBlocks(int32_t n) { m_instance->numberOfBlocks = Slot::makeInt32(n); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_CFG_HPP_