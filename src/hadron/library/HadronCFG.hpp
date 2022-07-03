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
 *
 *      /-----------------\
 *      |                 |
 *      v                 |
 *  +----------+      +----------+
 *  | CFGScope |----->| CFGFrame |
 *  +----------+      +----------+
 *       |  ^          ^  |  ^
 *       |  |          |  |  |
 *       |  | /--------/  |  |
 *       v  | |           v  |
 *  +----------+      +-------+
 *  | CFGBlock |----->| HIR   |
 *  +----------+      +-------+
 *       ^               |
 *       |               |
 *       \---------------/
 *
 * Using the CRTP prevents easy forward declaration of classes, making cyclical dependencies like this difficult to
 * resolve. The answer must be more templates!
 */

template<typename T, typename FrameT, typename BlockT>
class CFGScopeT : public Object<T, schema::HadronCFGScopeSchema> {
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

template<typename T, typename ScopeT, typename HIRT, typename BlockLiteralHIRT>
class CFGFrameT : public Object<T, schema::HadronCFGFrameSchema> {
public:
    CFGFrameT(): Object<T, schema::HadronCFGFrameSchema>() {}
    explicit CFGFrameT(schema::HadronCFGFrameSchema* instance): Object<T, schema::HadronCFGFrameSchema>(instance) {}
    explicit CFGFrameT(Slot instance): Object<T, schema::HadronCFGFrameSchema>(instance) {}
    ~CFGFrameT() {}

    static T makeCFGFrame(ThreadContext* context, BlockLiteralHIRT outerBlock, Method method) {
        auto frame = T::alloc(context);
        frame.initToNil();
        frame.setOuterBlockHIR(outerBlock);
        frame.setMethod(method);
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

    Method method() const {
        const T& t = static_cast<const T&>(*this);
        return Method(t.m_instance->method);
    }
    void setMethod(Method m) {
        T& t = static_cast<T&>(*this);
        t.m_instance->method = m.slot();
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

template<typename T, typename FrameT, typename ScopeT, typename PhiHIRT, typename HIRT>
class CFGBlockT : public Object<T, schema::HadronCFGBlockSchema> {
public:
    CFGBlockT(): Object<T, schema::HadronCFGBlockSchema>() {}
    explicit CFGBlockT(schema::HadronCFGBlockSchema* instance): Object<T, schema::HadronCFGBlockSchema>(instance) {}
    explicit CFGBlockT(Slot instance): Object<T, schema::HadronCFGBlockSchema>(instance) {}
    ~CFGBlockT() {}

    static T makeCFGBlock(ThreadContext* context, ScopeT scope, int32_t blockId) {
        auto block = T::alloc(context);
        block.initToNil();
        block.setScope(scope);
        block.setFrame(scope.frame());
        block.setId(blockId);
        block.setHasMethodReturn(false);
        block.setConstantValues(TypedIdentDict<Slot, HIRId>::makeTypedIdentDict(context));
        block.setConstantIds(TypedIdentSet<Integer>::makeTypedIdentSet(context));
        return block;
    }

    ScopeT scope() const {
        const T& t = static_cast<const T&>(*this);
        return ScopeT(t.m_instance->scope);
    }
    void setScope(ScopeT s) {
        T& t = static_cast<T&>(*this);
        t.m_instance->scope = s.slot();
    }

    FrameT frame() const {
        const T& t = static_cast<const T&>(*this);
        return FrameT(t.m_instance->frame);
    }
    void setFrame(FrameT f) {
        T& t = static_cast<T&>(*this);
        t.m_instance->frame = f.slot();
    }

    int32_t id() const {
        const T& t = static_cast<const T&>(*this);
        return t.m_instance->id.getInt32();
    }
    void setId(int32_t i) {
        T& t = static_cast<T&>(*this);
        t.m_instance->id = Slot::makeInt32(i);
    }

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

class CFGFrame;
class CFGBlock;
class CFGScope : public CFGScopeT<CFGScope, CFGFrame, CFGBlock> {
public:
    CFGScope(): CFGScopeT<CFGScope, CFGFrame, CFGBlock>() {}
    explicit CFGScope(schema::HadronCFGScopeSchema* instance): CFGScopeT<CFGScope, CFGFrame, CFGBlock>(instance) {}
    explicit CFGScope(Slot instance): CFGScopeT<CFGScope, CFGFrame, CFGBlock>(instance) {}
    ~CFGScope() {}
};

class HIR;
class BlockLiteralHIR;
class CFGFrame : public CFGFrameT<CFGFrame, CFGScope, HIR, BlockLiteralHIR> {
public:
    CFGFrame(): CFGFrameT<CFGFrame, CFGScope, HIR, BlockLiteralHIR>() {}
    explicit CFGFrame(schema::HadronCFGFrameSchema* instance):
            CFGFrameT<CFGFrame, CFGScope, HIR, BlockLiteralHIR>(instance) {}
    explicit CFGFrame(Slot instance): CFGFrameT<CFGFrame, CFGScope, HIR, BlockLiteralHIR>(instance) {}
    ~CFGFrame() {}
};

class PhiHIR;
class CFGBlock : public CFGBlockT<CFGBlock, CFGFrame, CFGScope, PhiHIR, HIR> {
public:
    CFGBlock(): CFGBlockT<CFGBlock, CFGFrame, CFGScope, PhiHIR, HIR>() {}
    explicit CFGBlock(schema::HadronCFGBlockSchema* instance):
            CFGBlockT<CFGBlock, CFGFrame, CFGScope, PhiHIR, HIR>(instance) {}
    explicit CFGBlock(Slot instance): CFGBlockT<CFGBlock, CFGFrame, CFGScope, PhiHIR, HIR>(instance) {}
    ~CFGBlock() {}

    // Adds hir to |statements| or |exitStatements|, returns assigned id.
    HIRId append(ThreadContext* context, HIR hir);
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_CFG_HPP_