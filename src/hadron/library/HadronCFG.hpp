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
Dependency graph of the various CFG objects and HIR:

      /-----------------\
      |                 |
      v                 |
 +----------+      +----------+
 | CFGScope |----->| CFGFrame |
 +----------+      +----------+
      |  ^          ^  |  ^
      |  |          |  |  |
      |  | /--------/  |  |
      v  | |           v  |
 +----------+      +-------+
 | CFGBlock |----->| HIR   |
 +----------+      +-------+
      ^               |
      |               |
      \---------------/

Using the CRTP prevents easy forward declaration of classes, making cyclical dependencies like this difficult to
resolve. The answer must be more templates!
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
        return scope;
    }

    static T makeSubCFGScope(ThreadContext* context, CFGScopeT parentScope) {
        auto scope = T::alloc(context);
        scope.initToNil();
        scope.setFrame(parentScope.frame());
        scope.setParent(parentScope);
        scope.setFrameIndex(0);
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

    CFGScopeT parent() const {
        const T& t = static_cast<const T&>(*this);
        return CFGScopeT(t.m_instance->parent);
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

    TypedArray<CFGScopeT> subScopes() const {
        const T& t = static_cast<const T&>(*this);
        return TypedArray<CFGScopeT>(t.m_instance->subScopes);
    }
    void setSubScopes(TypedArray<CFGScopeT> a) {
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
        frame.setRootScope(ScopeT::makeRootScope(context, frame));
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

template<typename T, typename FrameT, typename ScopeT>
class CFGBlockT : public Object<T, schema::HadronCFGBlockSchema> {
public:
    CFGBlockT(): Object<T, schema::HadronCFGBlockSchema>() {}
    explicit CFGBlockT(schema::HadronCFGBlockSchema* instance): Object<T, schema::HadronCFGBlockSchema>(instance) {}
    explicit CFGBlockT(Slot instance): Object<T, schema::HadronCFGBlockSchema>(instance) {}
    ~CFGBlockT() {}

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

    TypedIdentDict<Slot, Integer> constantValues() const {
        const T& t = static_cast<const T&>(*this);
        return TypedIdentDict<Slot, Integer>(t.m_instance->constantValues);
    }
    void setConstantValues(TypedIdentDict<Slot, Integer> tid) {
        T& t = static_cast<T&>(*this);
        t.m_instance->constantValues = tid.slot();
    }

    TypedIdentSet<Integer> constantIds() const {
        const T& t = static_cast<const T&>(*this);
        return TypedIdentSet<Integer>(t.m_instance->constantIds);
    }
    void setConstantIds(TypedIdentSet<Integer> tis) {
        T& t = static_cast<T&>(*this);
        t.m_instance->constantIds = tis.slot();
    }
};

class CFGFrame;
class CFGBlock;
class CFGScope : public CFGScopeT<CFGScope, CFGFrame, CFGBlock> {};

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

class CFGBlock : public CFGBlockT<CFGBlock, CFGFrame, CFGScope> {};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_CFG_HPP_