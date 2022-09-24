#ifndef SRC_HADRON_LIBRARY_HADRON_HIR_HPP_
#define SRC_HADRON_LIBRARY_HADRON_HIR_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/HadronCFG.hpp"
#include "hadron/library/Integer.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/Set.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/schema/HLang/HadronHIRSchema.hpp"

namespace hadron { namespace library {

template <typename T, typename S, typename B> class HIRBase : public Object<T, S> {
public:
    HIRBase(): Object<T, S>() { }
    explicit HIRBase(S* instance): Object<T, S>(instance) { }
    explicit HIRBase(Slot instance): Object<T, S>(instance) { }
    ~HIRBase() { }

    B toBase() const {
        const T& t = static_cast<const T&>(*this);
        return B::wrapUnsafe(Slot::makePointer(reinterpret_cast<library::Schema*>(t.m_instance)));
    }

    HIRId id() const {
        const T& t = static_cast<const T&>(*this);
        return HIRId(t.m_instance->id);
    }
    void setId(HIRId i) {
        T& t = static_cast<T&>(*this);
        t.m_instance->id = i.slot();
    }

    TypeFlags typeFlags() const {
        const T& t = static_cast<const T&>(*this);
        return static_cast<TypeFlags>(t.m_instance->typeFlags.getInt32());
    }
    void setTypeFlags(TypeFlags f) {
        T& t = static_cast<T&>(*this);
        t.m_instance->typeFlags = Slot::makeInt32(f);
    }

    TypedIdentSet<HIRId> reads() const {
        const T& t = static_cast<const T&>(*this);
        return TypedIdentSet<HIRId>(t.m_instance->reads);
    }
    void setReads(TypedIdentSet<HIRId> r) {
        T& t = static_cast<T&>(*this);
        t.m_instance->reads = r.slot();
    }

    TypedIdentSet<B> consumers() const {
        const T& t = static_cast<const T&>(*this);
        return TypedIdentSet<B>(t.m_instance->consumers);
    }
    void setConsumers(TypedIdentSet<B> c) {
        T& t = static_cast<T&>(*this);
        t.m_instance->consumers = c.slot();
    }

    CFGBlock owningBlock() const {
        const T& t = static_cast<const T&>(*this);
        return CFGBlock(t.m_instance->owningBlock);
    }
    void setOwningBlock(CFGBlock b) {
        T& t = static_cast<T&>(*this);
        t.m_instance->owningBlock = b.slot();
    }

protected:
    void initBase(ThreadContext* context, TypeFlags flags) {
        T& t = static_cast<T&>(*this);
        t.initToNil();
        t.setTypeFlags(flags);
        t.setReads(TypedIdentSet<HIRId>::makeTypedIdentSet(context));
        t.setConsumers(TypedIdentSet<B>::makeTypedIdentSet(context));
    }
};

class HIR : public HIRBase<HIR, schema::HadronHIRSchema, HIR> {
public:
    HIR(): HIRBase<HIR, schema::HadronHIRSchema, HIR>() { }
    explicit HIR(schema::HadronHIRSchema* instance): HIRBase<HIR, schema::HadronHIRSchema, HIR>(instance) { }
    explicit HIR(Slot instance): HIRBase<HIR, schema::HadronHIRSchema, HIR>(instance) { }
    ~HIR() { }

    // Recommended way to set the id in |id| member. Allows the HIR object to modify the proposed value type. For
    // convenience returns |value| as recorded within this object. Can return nil, which indicates that this operation /
    // only consumes values but doesn't generate a new one. When adding new HIR types, add them to this method.
    HIRId proposeId(HIRId proposedId);
};

class BlockLiteralHIR : public HIRBase<BlockLiteralHIR, schema::HadronBlockLiteralHIRSchema, HIR> {
public:
    BlockLiteralHIR(): HIRBase<BlockLiteralHIR, schema::HadronBlockLiteralHIRSchema, HIR>() { }
    explicit BlockLiteralHIR(schema::HadronBlockLiteralHIRSchema* instance):
        HIRBase<BlockLiteralHIR, schema::HadronBlockLiteralHIRSchema, HIR>(instance) { }
    explicit BlockLiteralHIR(Slot instance):
        HIRBase<BlockLiteralHIR, schema::HadronBlockLiteralHIRSchema, HIR>(instance) { }
    ~BlockLiteralHIR() { }

    static BlockLiteralHIR makeBlockLiteralHIR(ThreadContext* context) {
        auto blockLiteralHIR = BlockLiteralHIR::alloc(context);
        blockLiteralHIR.initBase(context, TypeFlags::kObjectFlag);
        return blockLiteralHIR;
    }

    CFGFrame frame() const { return CFGFrame(m_instance->frame); }
    void setFrame(CFGFrame f) { m_instance->frame = f.slot(); }

    FunctionDef functionDef() const { return FunctionDef(m_instance->functionDef); }
    void setFunctionDef(FunctionDef f) { m_instance->functionDef = f.slot(); }
};

class BranchHIR : public HIRBase<BranchHIR, schema::HadronBranchHIRSchema, HIR> {
public:
    BranchHIR(): HIRBase<BranchHIR, schema::HadronBranchHIRSchema, HIR>() { }
    explicit BranchHIR(schema::HadronBranchHIRSchema* instance):
        HIRBase<BranchHIR, schema::HadronBranchHIRSchema, HIR>(instance) { }
    explicit BranchHIR(Slot instance): HIRBase<BranchHIR, schema::HadronBranchHIRSchema, HIR>(instance) { }
    ~BranchHIR() { }

    static BranchHIR makeBranchHIR(ThreadContext* context) {
        auto branchHIR = BranchHIR::alloc(context);
        branchHIR.initBase(context, TypeFlags::kNoFlags);
        return branchHIR;
    }

    BlockId blockId() const { return BlockId(m_instance->blockId); }
    void setBlockId(BlockId i) { m_instance->blockId = i.slot(); }
};

class BranchIfTrueHIR : public HIRBase<BranchIfTrueHIR, schema::HadronBranchIfTrueHIRSchema, HIR> {
public:
    BranchIfTrueHIR(): HIRBase<BranchIfTrueHIR, schema::HadronBranchIfTrueHIRSchema, HIR>() { }
    explicit BranchIfTrueHIR(schema::HadronBranchIfTrueHIRSchema* instance):
        HIRBase<BranchIfTrueHIR, schema::HadronBranchIfTrueHIRSchema, HIR>(instance) { }
    explicit BranchIfTrueHIR(Slot instance):
        HIRBase<BranchIfTrueHIR, schema::HadronBranchIfTrueHIRSchema, HIR>(instance) { }
    ~BranchIfTrueHIR() { }

    static BranchIfTrueHIR makeBranchIfTrueHIR(ThreadContext* context, HIRId conditionId) {
        auto branchIfTrueHIR = BranchIfTrueHIR::alloc(context);
        branchIfTrueHIR.initBase(context, TypeFlags::kNoFlags);
        branchIfTrueHIR.setCondition(conditionId);
        branchIfTrueHIR.reads().typedAdd(context, conditionId);
        return branchIfTrueHIR;
    }

    HIRId condition() const { return HIRId(m_instance->condition); }
    void setCondition(HIRId i) { m_instance->condition = i.slot(); }

    BlockId blockId() const { return BlockId(m_instance->blockId); }
    void setBlockId(BlockId i) { m_instance->blockId = i.slot(); }
};

class ConstantHIR : public HIRBase<ConstantHIR, schema::HadronConstantHIRSchema, HIR> {
public:
    ConstantHIR(): HIRBase<ConstantHIR, schema::HadronConstantHIRSchema, HIR>() { }
    explicit ConstantHIR(schema::HadronConstantHIRSchema* instance):
        HIRBase<ConstantHIR, schema::HadronConstantHIRSchema, HIR>(instance) { }
    explicit ConstantHIR(Slot instance): HIRBase<ConstantHIR, schema::HadronConstantHIRSchema, HIR>(instance) { }
    ~ConstantHIR() { }

    static ConstantHIR makeConstantHIR(ThreadContext* context, Slot constantValue) {
        auto constantHIR = ConstantHIR::alloc(context);
        constantHIR.initBase(context, constantValue.getType());
        constantHIR.setConstant(constantValue);
        return constantHIR;
    }

    Slot constant() const { return m_instance->constant; }
    void setConstant(Slot c) { m_instance->constant = c; }
};

class LoadOuterFrameHIR : public HIRBase<LoadOuterFrameHIR, schema::HadronLoadOuterFrameHIRSchema, HIR> {
public:
    LoadOuterFrameHIR(): HIRBase<LoadOuterFrameHIR, schema::HadronLoadOuterFrameHIRSchema, HIR>() { }
    explicit LoadOuterFrameHIR(schema::HadronLoadOuterFrameHIRSchema* instance):
        HIRBase<LoadOuterFrameHIR, schema::HadronLoadOuterFrameHIRSchema, HIR>(instance) { }
    explicit LoadOuterFrameHIR(Slot instance):
        HIRBase<LoadOuterFrameHIR, schema::HadronLoadOuterFrameHIRSchema, HIR>(instance) { }
    ~LoadOuterFrameHIR() { }

    static LoadOuterFrameHIR makeOuterFrameHIR(ThreadContext* context, HIRId inner) {
        auto loadOuterFrameHIR = LoadOuterFrameHIR::alloc(context);
        loadOuterFrameHIR.initBase(context, TypeFlags::kObjectFlag);
        if (inner) {
            loadOuterFrameHIR.setInnerContext(inner);
            loadOuterFrameHIR.reads().typedAdd(context, inner);
        }
        return loadOuterFrameHIR;
    }

    HIRId innerContext() const { return HIRId(m_instance->innerContext); }
    void setInnerContext(HIRId i) { m_instance->innerContext = i.slot(); }
};

class MessageHIR : public HIRBase<MessageHIR, schema::HadronMessageHIRSchema, HIR> {
public:
    MessageHIR(): HIRBase<MessageHIR, schema::HadronMessageHIRSchema, HIR>() { }
    explicit MessageHIR(schema::HadronMessageHIRSchema* instance):
        HIRBase<MessageHIR, schema::HadronMessageHIRSchema, HIR>(instance) { }
    explicit MessageHIR(Slot instance): HIRBase<MessageHIR, schema::HadronMessageHIRSchema, HIR>(instance) { }
    ~MessageHIR() { }

    static MessageHIR makeMessageHIR(ThreadContext* context) {
        auto messageHIR = MessageHIR::alloc(context);
        messageHIR.initBase(context, TypeFlags::kAllFlags);
        return messageHIR;
    }

    void addArgument(ThreadContext* context, HIRId arg) {
        reads().typedAdd(context, arg);
        setArguments(arguments().typedAdd(context, arg));
    }

    // TODO: why not force these into pairs?
    void addKeywordArgument(ThreadContext* context, HIRId arg) {
        reads().typedAdd(context, arg);
        setKeywordArguments(keywordArguments().typedAdd(context, arg));
    }

    Symbol selector(ThreadContext* context) const { return Symbol(context, m_instance->selector); }
    void setSelector(Symbol s) { m_instance->selector = s.slot(); }

    TypedArray<HIRId> arguments() const { return TypedArray<HIRId>(m_instance->arguments); }
    void setArguments(TypedArray<HIRId> a) { m_instance->arguments = a.slot(); }

    TypedArray<HIRId> keywordArguments() const { return TypedArray<HIRId>(m_instance->keywordArguments); }
    void setKeywordArguments(TypedArray<HIRId> a) { m_instance->keywordArguments = a.slot(); }
};

class MethodReturnHIR : public HIRBase<MethodReturnHIR, schema::HadronMethodReturnHIRSchema, HIR> {
public:
    MethodReturnHIR(): HIRBase<MethodReturnHIR, schema::HadronMethodReturnHIRSchema, HIR>() { }
    explicit MethodReturnHIR(schema::HadronMethodReturnHIRSchema* instance):
        HIRBase<MethodReturnHIR, schema::HadronMethodReturnHIRSchema, HIR>(instance) { }
    explicit MethodReturnHIR(Slot instance):
        HIRBase<MethodReturnHIR, schema::HadronMethodReturnHIRSchema, HIR>(instance) { }
    ~MethodReturnHIR() { }

    static MethodReturnHIR makeMethodReturnHIR(ThreadContext* context, HIRId retVal) {
        auto methodReturnHIR = MethodReturnHIR::alloc(context);
        methodReturnHIR.initBase(context, TypeFlags::kNoFlags);
        methodReturnHIR.reads().typedAdd(context, retVal);
        methodReturnHIR.setReturnValue(retVal);
        return methodReturnHIR;
    }

    HIRId returnValue() const { return HIRId(m_instance->returnValue); }
    void setReturnValue(HIRId i) { m_instance->returnValue = i.slot(); }
};

class PhiHIR : public HIRBase<PhiHIR, schema::HadronPhiHIRSchema, HIR> {
public:
    PhiHIR(): HIRBase<PhiHIR, schema::HadronPhiHIRSchema, HIR>() { }
    explicit PhiHIR(schema::HadronPhiHIRSchema* instance):
        HIRBase<PhiHIR, schema::HadronPhiHIRSchema, HIR>(instance) { }
    explicit PhiHIR(Slot instance): HIRBase<PhiHIR, schema::HadronPhiHIRSchema, HIR>(instance) { }
    ~PhiHIR() { }

    static PhiHIR makePhiHIR(ThreadContext* context) {
        auto phiHIR = PhiHIR::alloc(context);
        phiHIR.initBase(context, TypeFlags::kNoFlags);
        phiHIR.setIsSelfReferential(false);
        return phiHIR;
    }

    void addInput(ThreadContext* context, HIR input);
    // A phi is *trivial* if it has only one distinct input value that is not self-referential. If this phi is trivial,
    // return the trivial value. Otherwise return a nil value.
    HIRId getTrivialValue() const;

    Symbol name(ThreadContext* context) const { return Symbol(context, m_instance->name); }
    void setName(Symbol n) { m_instance->name = n.slot(); }

    TypedArray<Integer> inputs() const { return TypedArray<Integer>(m_instance->inputs); }
    void setInputs(TypedArray<Integer> a) { m_instance->inputs = a.slot(); }

    bool isSelfReferential() const { return m_instance->isSelfReferential.getBool(); }
    void setIsSelfReferential(bool b) { m_instance->isSelfReferential = Slot::makeBool(b); }
};

class ReadFromClassHIR : public HIRBase<ReadFromClassHIR, schema::HadronReadFromClassHIRSchema, HIR> {
public:
    ReadFromClassHIR(): HIRBase<ReadFromClassHIR, schema::HadronReadFromClassHIRSchema, HIR>() { }
    explicit ReadFromClassHIR(schema::HadronReadFromClassHIRSchema* instance):
        HIRBase<ReadFromClassHIR, schema::HadronReadFromClassHIRSchema, HIR>(instance) { }
    explicit ReadFromClassHIR(Slot instance):
        HIRBase<ReadFromClassHIR, schema::HadronReadFromClassHIRSchema, HIR>(instance) { }
    ~ReadFromClassHIR() { }

    static ReadFromClassHIR makeReadFromClassHIR(ThreadContext* context, HIRId classArray, int32_t index, Symbol name) {
        auto readFromClassHIR = ReadFromClassHIR::alloc(context);
        readFromClassHIR.initBase(context, TypeFlags::kAllFlags);
        readFromClassHIR.setClassVariableArray(classArray);
        readFromClassHIR.reads().typedAdd(context, classArray);
        readFromClassHIR.setArrayIndex(index);
        readFromClassHIR.setValueName(name);
        return readFromClassHIR;
    }

    HIRId classVariableArray() const { return HIRId(m_instance->classVariableArray); }
    void setClassVariableArray(HIRId i) { m_instance->classVariableArray = i.slot(); }

    int32_t arrayIndex() const { return m_instance->arrayIndex.getInt32(); }
    void setArrayIndex(int32_t i) { m_instance->arrayIndex = Slot::makeInt32(i); }

    Symbol valueName(ThreadContext* context) const { return Symbol(context, m_instance->valueName); }
    void setValueName(Symbol s) { m_instance->valueName = s.slot(); }
};

class ReadFromContextHIR : public HIRBase<ReadFromContextHIR, schema::HadronReadFromContextHIRSchema, HIR> {
public:
    ReadFromContextHIR(): HIRBase<ReadFromContextHIR, schema::HadronReadFromContextHIRSchema, HIR>() { }
    explicit ReadFromContextHIR(schema::HadronReadFromContextHIRSchema* instance):
        HIRBase<ReadFromContextHIR, schema::HadronReadFromContextHIRSchema, HIR>(instance) { }
    explicit ReadFromContextHIR(Slot instance):
        HIRBase<ReadFromContextHIR, schema::HadronReadFromContextHIRSchema, HIR>(instance) { }
    ~ReadFromContextHIR() { }

    static ReadFromContextHIR makeReadFromContextHIR(ThreadContext* context, int32_t off, Symbol name) {
        auto readFromContextHIR = ReadFromContextHIR::alloc(context);
        readFromContextHIR.initBase(context, TypeFlags::kAllFlags);
        readFromContextHIR.setOffset(off);
        readFromContextHIR.setValueName(name);
        return readFromContextHIR;
    }

    int32_t offset() const { return m_instance->offset.getInt32(); }
    void setOffset(int32_t i) { m_instance->offset = Slot::makeInt32(i); }

    Symbol valueName(ThreadContext* context) const { return Symbol(context, m_instance->valueName); }
    void setValueName(Symbol s) { m_instance->valueName = s.slot(); }
};

class ReadFromFrameHIR : public HIRBase<ReadFromFrameHIR, schema::HadronReadFromFrameHIRSchema, HIR> {
public:
    ReadFromFrameHIR(): HIRBase<ReadFromFrameHIR, schema::HadronReadFromFrameHIRSchema, HIR>() { }
    explicit ReadFromFrameHIR(schema::HadronReadFromFrameHIRSchema* instance):
        HIRBase<ReadFromFrameHIR, schema::HadronReadFromFrameHIRSchema, HIR>(instance) { }
    explicit ReadFromFrameHIR(Slot instance):
        HIRBase<ReadFromFrameHIR, schema::HadronReadFromFrameHIRSchema, HIR>(instance) { }
    ~ReadFromFrameHIR() { }

    // If |framePointer| is nil this will use the current active frame pointer.
    static ReadFromFrameHIR makeReadFromFrameHIR(ThreadContext* context, int32_t index, HIRId framePointer,
                                                 Symbol name) {
        auto readFromFrameHIR = ReadFromFrameHIR::alloc(context);
        readFromFrameHIR.initBase(context, TypeFlags::kAllFlags);
        readFromFrameHIR.setFrameIndex(index);
        if (framePointer) {
            readFromFrameHIR.setFrameId(framePointer);
            readFromFrameHIR.reads().typedAdd(context, framePointer);
        }
        readFromFrameHIR.setValueName(name);
        return readFromFrameHIR;
    }

    int32_t frameIndex() const { return m_instance->frameIndex.getInt32(); }
    void setFrameIndex(int32_t i) { m_instance->frameIndex = Slot::makeInt32(i); }

    HIRId frameId() const { return HIRId(m_instance->frameId); }
    void setFrameId(HIRId i) { m_instance->frameId = i.slot(); }

    Symbol valueName(ThreadContext* context) const { return Symbol(context, m_instance->valueName); }
    void setValueName(Symbol s) { m_instance->valueName = s.slot(); }
};

class ReadFromThisHIR : public HIRBase<ReadFromThisHIR, schema::HadronReadFromThisHIRSchema, HIR> {
public:
    ReadFromThisHIR(): HIRBase<ReadFromThisHIR, schema::HadronReadFromThisHIRSchema, HIR>() { }
    explicit ReadFromThisHIR(schema::HadronReadFromThisHIRSchema* instance):
        HIRBase<ReadFromThisHIR, schema::HadronReadFromThisHIRSchema, HIR>(instance) { }
    explicit ReadFromThisHIR(Slot instance):
        HIRBase<ReadFromThisHIR, schema::HadronReadFromThisHIRSchema, HIR>(instance) { }
    ~ReadFromThisHIR() { }

    // TODO: lots of redundant naming here, maybe move the make* methods outside of the classes? They aren't accessing
    // anything private..
    static ReadFromThisHIR makeReadFromThisHIR(ThreadContext* context, HIRId tID, int32_t index, Symbol name) {
        auto readFromThisHIR = ReadFromThisHIR::alloc(context);
        readFromThisHIR.initBase(context, TypeFlags::kAllFlags);
        readFromThisHIR.setThisId(tID);
        readFromThisHIR.reads().typedAdd(context, tID);
        readFromThisHIR.setIndex(index);
        readFromThisHIR.setValueName(name);
        return readFromThisHIR;
    }

    HIRId thisId() const { return HIRId(m_instance->thisId); }
    void setThisId(HIRId i) { m_instance->thisId = i.slot(); }

    int32_t index() const { return m_instance->index.getInt32(); }
    void setIndex(int32_t i) { m_instance->index = Slot::makeInt32(i); }

    Symbol valueName(ThreadContext* context) const { return Symbol(context, m_instance->valueName); }
    void setValueName(Symbol s) { m_instance->valueName = s.slot(); }
};

class RouteToSuperclassHIR : public HIRBase<RouteToSuperclassHIR, schema::HadronRouteToSuperclassHIRSchema, HIR> {
public:
    RouteToSuperclassHIR(): HIRBase<RouteToSuperclassHIR, schema::HadronRouteToSuperclassHIRSchema, HIR>() { }
    explicit RouteToSuperclassHIR(schema::HadronRouteToSuperclassHIRSchema* instance):
        HIRBase<RouteToSuperclassHIR, schema::HadronRouteToSuperclassHIRSchema, HIR>(instance) { }
    explicit RouteToSuperclassHIR(Slot instance):
        HIRBase<RouteToSuperclassHIR, schema::HadronRouteToSuperclassHIRSchema, HIR>(instance) { }
    ~RouteToSuperclassHIR() { }

    static RouteToSuperclassHIR makeRouteToSuperclassHIR(ThreadContext* context, HIRId tId) {
        auto routeToSuperclassHIR = RouteToSuperclassHIR::alloc(context);
        routeToSuperclassHIR.initBase(context, TypeFlags::kAllFlags);
        routeToSuperclassHIR.setThisId(tId);
        routeToSuperclassHIR.reads().typedAdd(context, tId);
        return routeToSuperclassHIR;
    }

    HIRId thisId() const { return HIRId(m_instance->thisId); }
    void setThisId(HIRId i) { m_instance->thisId = i.slot(); }
};

class WriteToClassHIR : public HIRBase<WriteToClassHIR, schema::HadronWriteToClassHIRSchema, HIR> {
public:
    WriteToClassHIR(): HIRBase<WriteToClassHIR, schema::HadronWriteToClassHIRSchema, HIR>() { }
    explicit WriteToClassHIR(schema::HadronWriteToClassHIRSchema* instance):
        HIRBase<WriteToClassHIR, schema::HadronWriteToClassHIRSchema, HIR>(instance) { }
    explicit WriteToClassHIR(Slot instance):
        HIRBase<WriteToClassHIR, schema::HadronWriteToClassHIRSchema, HIR>(instance) { }
    ~WriteToClassHIR() { }

    static WriteToClassHIR makeWriteToClassHIR(ThreadContext* context, HIRId classArray, int32_t index, Symbol name,
                                               HIRId v) {
        auto writeToClassHIR = WriteToClassHIR::alloc(context);
        writeToClassHIR.initBase(context, TypeFlags::kNoFlags);
        writeToClassHIR.setClassVariableArray(classArray);
        writeToClassHIR.reads().typedAdd(context, classArray);
        writeToClassHIR.setArrayIndex(index);
        writeToClassHIR.setValueName(name);
        writeToClassHIR.setToWrite(v);
        writeToClassHIR.reads().typedAdd(context, v);
        return writeToClassHIR;
    }

    HIRId classVariableArray() const { return HIRId(m_instance->classVariableArray); }
    void setClassVariableArray(HIRId i) { m_instance->classVariableArray = i.slot(); }

    int32_t arrayIndex() const { return m_instance->arrayIndex.getInt32(); }
    void setArrayIndex(int32_t i) { m_instance->arrayIndex = Slot::makeInt32(i); }

    Symbol valueName(ThreadContext* context) const { return Symbol(context, m_instance->valueName); }
    void setValueName(Symbol s) { m_instance->valueName = s.slot(); }

    HIRId toWrite() const { return HIRId(m_instance->toWrite); }
    void setToWrite(HIRId i) { m_instance->toWrite = i.slot(); }
};

class WriteToFrameHIR : public HIRBase<WriteToFrameHIR, schema::HadronWriteToFrameHIRSchema, HIR> {
public:
    WriteToFrameHIR(): HIRBase<WriteToFrameHIR, schema::HadronWriteToFrameHIRSchema, HIR>() { }
    explicit WriteToFrameHIR(schema::HadronWriteToFrameHIRSchema* instance):
        HIRBase<WriteToFrameHIR, schema::HadronWriteToFrameHIRSchema, HIR>(instance) { }
    explicit WriteToFrameHIR(Slot instance):
        HIRBase<WriteToFrameHIR, schema::HadronWriteToFrameHIRSchema, HIR>(instance) { }
    ~WriteToFrameHIR() { }

    static WriteToFrameHIR makeWriteToFrameHIR(ThreadContext* context, int32_t index, HIRId framePointer, Symbol name,
                                               HIRId v) {
        auto writeToFrameHIR = WriteToFrameHIR::alloc(context);
        writeToFrameHIR.initBase(context, TypeFlags::kNoFlags);
        writeToFrameHIR.setFrameIndex(index);
        if (framePointer) {
            writeToFrameHIR.setFrameId(framePointer);
            writeToFrameHIR.reads().typedAdd(context, framePointer);
        }
        writeToFrameHIR.setValueName(name);
        writeToFrameHIR.setToWrite(v);
        writeToFrameHIR.reads().typedAdd(context, v);
        return writeToFrameHIR;
    }

    int32_t frameIndex() const { return m_instance->frameIndex.getInt32(); }
    void setFrameIndex(int32_t i) { m_instance->frameIndex = Slot::makeInt32(i); }

    HIRId frameId() const { return HIRId(m_instance->frameId); }
    void setFrameId(HIRId i) { m_instance->frameId = i.slot(); }

    Symbol valueName(ThreadContext* context) const { return Symbol(context, m_instance->valueName); }
    void setValueName(Symbol s) { m_instance->valueName = s.slot(); }

    HIRId toWrite() const { return HIRId(m_instance->toWrite); }
    void setToWrite(HIRId i) { m_instance->toWrite = i.slot(); }
};

class WriteToThisHIR : public HIRBase<WriteToThisHIR, schema::HadronWriteToThisHIRSchema, HIR> {
public:
    WriteToThisHIR(): HIRBase<WriteToThisHIR, schema::HadronWriteToThisHIRSchema, HIR>() { }
    explicit WriteToThisHIR(schema::HadronWriteToThisHIRSchema* instance):
        HIRBase<WriteToThisHIR, schema::HadronWriteToThisHIRSchema, HIR>(instance) { }
    explicit WriteToThisHIR(Slot instance):
        HIRBase<WriteToThisHIR, schema::HadronWriteToThisHIRSchema, HIR>(instance) { }
    ~WriteToThisHIR() { }

    static WriteToThisHIR makeWriteToThisHIR(ThreadContext* context, HIRId tID, int32_t idx, Symbol name, HIRId v) {
        auto writeToThisHIR = WriteToThisHIR::alloc(context);
        writeToThisHIR.initBase(context, TypeFlags::kNoFlags);
        writeToThisHIR.setThisId(tID);
        writeToThisHIR.reads().typedAdd(context, tID);
        writeToThisHIR.setIndex(idx);
        writeToThisHIR.setValueName(name);
        writeToThisHIR.setToWrite(v);
        writeToThisHIR.reads().typedAdd(context, v);
        return writeToThisHIR;
    }

    HIRId thisId() const { return HIRId(m_instance->thisId); }
    void setThisId(HIRId i) { m_instance->thisId = i.slot(); }

    int32_t index() const { return m_instance->index.getInt32(); }
    void setIndex(int32_t i) { m_instance->index = Slot::makeInt32(i); }

    Symbol valueName(ThreadContext* context) const { return Symbol(context, m_instance->valueName); }
    void setValueName(Symbol s) { m_instance->valueName = s.slot(); }

    HIRId toWrite() const { return HIRId(m_instance->toWrite); }
    void setToWrite(HIRId i) { m_instance->toWrite = i.slot(); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_HIR_HPP_