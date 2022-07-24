#ifndef SRC_HADRON_LIBRARY_HADRON_CFG_HPP_
#define SRC_HADRON_LIBRARY_HADRON_CFG_HPP_

#include "hadron/library/Dictionary.hpp"
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
 * +---------+
 * | CFGCope |
 * +---------+
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
    HIRId append(ThreadContext* context, HIR hir);

    int32_t id() const { return m_instance->id.getInt32(); }
    void setId(int32_t i) { m_instance->id = Slot::makeInt32(i); }

    TypedArray<T> predecessors() const {
        const T& t = static_cast<const T&>(*this);
        return TypedArray<T>(t.m_instance->predecessors);
    }
    void setPredecessors(TypedArray<T> a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->predecessors = a.slot();
    }

    TypedArray<T> successors() const {
        const T& t = static_cast<const T&>(*this);
        return TypedArray<T>(t.m_instance->successors);
    }
    void setSuccessors(TypedArray<T> a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->successors = a.slot();
    }

    TypedArray<PhiHIRT> phis() const {
        const T& t = static_cast<const T&>(*this);
        return TypedArray<PhiHIRT>(t.m_instance->phis);
    }
    void setPhis(TypedArray<PhiHIRT> a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->phis = a.slot();
    }

    TypedArray<HIRT> statements() const {
        const T& t = static_cast<const T&>(*this);
        return TypedArray<HIRT>(t.m_instance->statements);
    }
    void setStatements(TypedArray<HIRT> a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->statements = a.slot();
    }

    TypedArray<HIRT> exitStatements() const {
        const T& t = static_cast<const T&>(*this);
        return TypedArray<HIRT>(t.m_instance->exitStatements);
    }
    void setExitStatements(TypedArray<HIRT> a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->exitStatements = a.slot();
    }

    bool hasMethodReturn() const {
        const T& t = static_cast<const T&>(*this);
        return t.m_instance->hasMethodReturn.getBool();
    }
    void setHasMethodReturn(bool b) {
        T& t = static_cast<T&>(*this);
        t.m_instance->hasMethodReturn = Slot::makeBool(b);
    }

    HIRId finalValue() const {
        const T& t = static_cast<const T&>(*this);
        return HIRId(t.m_instance->finalValue);
    }
    void setFinalValue(HIRId id) {
        T& t = static_cast<T&>(*this);
        t.m_instance->finalValue = id.slot();
    }

    TypedIdentDict<Slot, HIRId> constantValues() const {
        const T& t = static_cast<const T&>(*this);
        return TypedIdentDict<Slot, HIRId>(t.m_instance->constantValues);
    }
    void setConstantValues(TypedIdentDict<Slot, Integer> tid) {
        T& t = static_cast<T&>(*this);
        t.m_instance->constantValues = tid.slot();
    }

    HIRId nilConstantValue() const {
        const T& t = static_cast<const T&>(*this);
        return HIRId(t.m_instance->nilConstantValue);
    }
    void setNilConstantValue(HIRId i) {
        T& t = static_cast<T&>(*this);
        t.m_instance->nilConstantValue = i.slot();
    }

    TypedIdentSet<Integer> constantIds() const {
        const T& t = static_cast<const T&>(*this);
        return TypedIdentSet<Integer>(t.m_instance->constantIds);
    }
    void setConstantIds(TypedIdentSet<Integer> tis) {
        T& t = static_cast<T&>(*this);
        t.m_instance->constantIds = tis.slot();
    }

    Integer loopReturnPredIndex() const {
        const T& t = static_cast<const T&>(*this);
        return Integer(t.m_instance->loopReturnPredIndex);
    }
    void setLoopReturnPredIndex(Integer i) {
        T& t = static_cast<T&>(*this);
        t.m_instance->loopReturnPredIndex = i.slot();
    }
};



class CFGScope : public Object<CFGScope, schema::HadronCFGScopeSchema> {
public:
    CFGScopeT(): Object<T, schema::HadronCFGScopeSchema>() {}
    explicit CFGScopeT(schema::HadronCFGScopeSchema* instance): Object<T, schema::HadronCFGScopeSchema>(instance) {}
    explicit CFGScopeT(Slot instance): Object<T, schema::HadronCFGScopeSchema>(instance) {}
    ~CFGScopeT() {}

    static T makeRootCFGScope(ThreadContext* context, FrameT owningFrame) {
        auto scope = T::alloc(context);
        scope.initToNil();
        scope.setFrame(owningFrame);
        scope.setFrameIndex(0);
        scope.setValueIndices(TypedIdentDict<Symbol, Integer>::makeTypedIdentDict(context));
        return scope;
    }

    static T makeSubCFGScope(ThreadContext* context, T parentScope) {
        auto scope = T::alloc(context);
        scope.initToNil();
        scope.setFrame(parentScope.frame());
        scope.setParent(parentScope);
        scope.setFrameIndex(0);
        scope.setValueIndices(TypedIdentDict<Symbol, Integer>::makeTypedIdentDict(context));
        return scope;
    }

    FrameT frame() const {
        const T& t = static_cast<const T&>(*this);
        return FrameT(t.m_instance->frame);
    }
    void setFrame(FrameT f) {
        T& t = static_cast<T&>(*this);
        t.m_instance->frame = f.slot();
    }

    T parent() const {
        const T& t = static_cast<const T&>(*this);
        return T(t.m_instance->parent);
    }
    void setParent(CFGScopeT p) {
        T& t = static_cast<T&>(*this);
        t.m_instance->parent = p.slot();
    }

    TypedArray<BlockT> blocks() const {
        const T& t = static_cast<const T&>(*this);
        return TypedArray<BlockT>(t.m_instance->blocks);
    }
    void setBlocks(TypedArray<BlockT> a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->blocks = a.slot();
    }

    TypedArray<T> subScopes() const {
        const T& t = static_cast<const T&>(*this);
        return TypedArray<T>(t.m_instance->subScopes);
    }
    void setSubScopes(TypedArray<T> a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->subScopes = a.slot();
    }

    int32_t frameIndex() const {
        const T& t = static_cast<const T&>(*this);
        return t.m_instance->frameIndex.getInt32();
    }
    void setFrameIndex(int32_t i) {
        T& t = static_cast<T&>(*this);
        t.m_instance->frameIndex = Slot::makeInt32(i);
    }

    TypedIdentDict<Symbol, Integer> valueIndices() const {
        const T& t = static_cast<const T&>(*this);
        return TypedIdentDict<Symbol, Integer>(t.m_instance->valueIndices);
    }
    void setValueIndices(TypedIdentDict<Symbol, Integer> tid) {
        T& t = static_cast<T&>(*this);
        t.m_instance->valueIndices = tid.slot();
    }
};

class CFGFrame : public Object<CFGFrame, schema::HadronCFGFrameSchema> {
public:
    CFGFrame(): Object<CFGFrame, schema::HadronCFGFrameSchema>() {}
    explicit CFGFrame(schema::HadronCFGFrameSchema* instance):
            Object<CFGFrame, schema::HadronCFGFrameSchema>(instance) {}
    explicit CFGFrame(Slot instance): Object<CFGFrame, schema::HadronCFGFrameSchema>(instance) {}
    ~CFGFrame() {}

    static CFGFrame makeCFGFrame(ThreadContext* context) {
        auto frame = CFGrame::alloc(context);
        frame.initToNil();
        frame.setOuterBlockHIR(outerBlock);
        frame.setHasVarArgs(false);
        frame.setRootScope(ScopeT::makeRootCFGScope(context, frame));
        frame.setNumberOfBlocks(0);
        return frame;
    }

    BlockLiteralHIRT outerBlockHIR() const {
        const T& t = static_cast<const T&>(*this);
        return BlockLiteralHIRT(t.m_instance->outerBlockHIR);
    }
    void setOuterBlockHIR(BlockLiteralHIRT b) {
        T& t = static_cast<T&>(*this);
        t.m_instance->outerBlockHIR = b.slot();
    }

    bool hasVarArgs() const {
        const T& t = static_cast<const T&>(*this);
        return t.m_instance->hasVarArgs.getBool();
    }
    void setHasVarArgs(bool b) {
        T& t = static_cast<T&>(*this);
        t.m_instance->hasVarArgs = Slot::makeBool(b);
    }

    SymbolArray variableNames() const {
        const T& t = static_cast<const T&>(*this);
        return SymbolArray(t.m_instance->variableNames);
    }
    void setVariableNames(SymbolArray a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->variableNames = a.slot();
    }

    Array prototypeFrame() const {
        const T& t = static_cast<const T&>(*this);
        return Array(t.m_instance->prototypeFrame);
    }
    void setPrototypeFrame(Array a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->prototypeFrame = a.slot();
    }

    TypedArray<BlockLiteralHIRT> innerBlocks() const {
        const T& t = static_cast<const T&>(*this);
        return TypedArray<BlockLiteralHIRT>(t.m_instance->innerBlocks);
    }
    void setInnerBlocks(TypedArray<BlockLiteralHIRT> a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->innerBlocks = a.slot();
    }

    FunctionDefArray selectors() const {
        const T& t = static_cast<const T&>(*this);
        return FunctionDefArray(t.m_instance->selectors);
    }
    void setSelectors(FunctionDefArray a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->selectors = a.slot();
    }

    ScopeT rootScope() const {
        const T& t = static_cast<const T&>(*this);
        return ScopeT(t.m_instance->rootScope);
    }
    void setRootScope(ScopeT s) {
        T& t = static_cast<T&>(*this);
        t.m_instance->rootScope = s.slot();
    }

    TypedArray<HIRT> values() const {
        const T& t = static_cast<const T&>(*this);
        return TypedArray<HIRT>(t.m_instance->values);
    }
    void setValues(TypedArray<HIRT> a) {
        T& t = static_cast<T&>(*this);
        t.m_instance->values = a.slot();
    }

    int32_t numberOfBlocks() const {
        const T& t = static_cast<const T&>(*this);
        return t.m_instance->numberOfBlocks.getInt32();
    }
    void setNumberOfBlocks(int32_t n) {
        T& t = static_cast<T&>(*this);
        t.m_instance->numberOfBlocks = Slot::makeInt32(n);
    }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_CFG_HPP_