#include "hadron/Pipeline.hpp"

#include "hadron/Arch.hpp"
#include "hadron/Block.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/Emitter.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Hash.hpp"
#include "hadron/HIR.hpp"
#include "hadron/Heap.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/Parser.hpp"
#include "hadron/RegisterAllocator.hpp"
#include "hadron/Resolver.hpp"
#include "hadron/Scope.hpp"
#include "hadron/ThreadContext.hpp"
#include "hadron/VirtualJIT.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

Pipeline::Pipeline(): m_errorReporter(std::make_shared<ErrorReporter>()) { setDefaults(); }

Pipeline::Pipeline(std::shared_ptr<ErrorReporter> errorReporter): m_errorReporter(errorReporter) { setDefaults(); }

Pipeline::~Pipeline() {}

library::FunctionDef Pipeline::compileCode(ThreadContext* context, std::string_view code) {

    Lexer lexer(code, m_errorReporter);
    if (!lexer.lex()) { return library::FunctionDef(nullptr); }

    Parser parser(&lexer, m_errorReporter);
    if (!parser.parse()) { return library::FunctionDef(nullptr); }
    assert(parser.root()->nodeType == parse::NodeType::kBlock);

    auto functionDef = library::FunctionDef::alloc(context);
    buildBlock(context, functionDef, reinterpret_cast<const parse::BlockNode*>(parser.root()), &lexer);
    return functionDef;
}

library::FunctionDef Pipeline::compileBlock(ThreadContext* context, const parse::BlockNode* blockNode,
        const Lexer* lexer) {
    auto functionDef = library::FunctionDef::alloc(context);
    buildBlock(context, functionDef, blockNode, lexer);
    return functionDef;
}

library::Method Pipeline::compileMethod(ThreadContext* context, const parse::MethodNode* methodNode,
        const Lexer* lexer, const library::Class /* classDef */) {
    auto method = library::Method::alloc(context);
    buildBlock(context, library::FunctionDef::wrapUnsafe(method.instance()), methodNode->body.get(), lexer);
    return method;
}

#if HADRON_PIPELINE_VALIDATE
bool Pipeline::afterBlockBuilder(const Frame*, const parse::BlockNode*, const Lexer*) { return true; }
bool Pipeline::afterBlockSerializer(const LinearBlock*) { return true; }
bool Pipeline::afterLifetimeAnalyzer(const LinearBlock*) { return true; }
bool Pipeline::afterRegisterAllocator(const LinearBlock*) { return true; }
bool Pipeline::afterResolver(const LinearBlock*) { return true; }
bool Pipeline::afterEmitter(const LinearBlock*, library::Int8Array) { return true; }
#endif // HADRON_PIPELINE_VALIDATE

void Pipeline::setDefaults() {
    m_numberOfRegisters = kNumberOfPhysicalRegisters;
    m_jitToVirtualMachine = false;
}

bool Pipeline::buildBlock(ThreadContext* context, library::FunctionDef functionDef, const parse::BlockNode* blockNode,
        const Lexer* lexer) {
    hadron::BlockBuilder builder(lexer, m_errorReporter);
    auto frame = builder.buildFrame(context, blockNode);
    if (!frame) { return false; }
#if HADRON_PIPELINE_VALIDATE
    if (!validateFrame(context, frame.get(), blockNode, lexer)) { return false; }
    if (!afterBlockBuilder(frame.get(), blockNode, lexer)) { return false; }
    size_t numberOfBlocks = static_cast<size_t>(frame->numberOfBlocks);
    size_t numberOfValues = frame->numberOfValues;
#endif // HADRON_PIPELINE_VALIDATE

    functionDef.setArgNames(frame->argumentOrder);
    functionDef.setPrototypeFrame(frame->argumentDefaults);

    BlockSerializer serializer;
    auto linearBlock = serializer.serialize(std::move(frame));
    if (!linearBlock) { return false; }
#if HADRON_PIPELINE_VALIDATE
    if (!validateSerializedBlock(linearBlock.get(), numberOfBlocks, numberOfValues)) { return false; }
    if (!afterBlockSerializer(linearBlock.get())) { return false; }
#endif // HADRON_PIPELINE_VALIDATE

    LifetimeAnalyzer analyzer;
    analyzer.buildLifetimes(linearBlock.get());
#if HADRON_PIPELINE_VALIDATE
    if (!validateLifetimes(linearBlock.get())) { return false; }
    if (!afterLifetimeAnalyzer(linearBlock.get())) { return false; }
#endif // HADRON_PIPELINE_VALIDATE

    RegisterAllocator allocator(m_numberOfRegisters);
    allocator.allocateRegisters(linearBlock.get());
#if HADRON_PIPELINE_VALIDATE
    if (!validateAllocation(linearBlock.get())) { return false; }
    if (!afterRegisterAllocator(linearBlock.get())) { return false; }
#endif // HADRON_PIPELINE_VALIDATE

    Resolver resolver;
    resolver.resolve(linearBlock.get());
#if HADRON_PIPELINE_VALIDATE
    if (!validateResolution(linearBlock.get())) { return false; }
    if (!afterResolver(linearBlock.get())) { return false; }
#endif // HADRON_PIPELINE_VALIDATE

    size_t jitMaxSize = sizeof(library::Int8Array);
    for (const auto& hir : linearBlock->instructions) {
        jitMaxSize += 16 + (16 * hir->moves.size());
    }
    std::unique_ptr<JIT> jit;
    library::Int8Array bytecodeArray;
    if (m_jitToVirtualMachine) {
        jit = std::make_unique<VirtualJIT>(m_numberOfRegisters, m_numberOfRegisters);
        bytecodeArray = library::Int8Array::arrayAlloc(context, jitMaxSize);
        jitMaxSize = context->heap->getAllocationSize(bytecodeArray.instance());
    } else {
        LighteningJIT::markThreadForJITCompilation();
        jit = std::make_unique<LighteningJIT>();
        size_t allocationSize = 0;
        bytecodeArray = library::Int8Array::arrayAllocJIT(context, jitMaxSize, allocationSize);
        jitMaxSize = allocationSize;
    }
    jit->begin(bytecodeArray.start(), bytecodeArray.capacity(context));

    Emitter emitter;
    emitter.emit(linearBlock.get(), jit.get());
    assert(!jit->hasJITBufferOverflow());
    size_t finalSize = 0;
    jit->end(&finalSize);
    bytecodeArray.resize(context, finalSize);

#if HADRON_PIPELINE_VALIDATE
    if (!validateEmission(linearBlock.get(), bytecodeArray)) { return false; }
    if (!afterEmitter(linearBlock.get(), bytecodeArray)) { return false; }
#endif

    functionDef.setCode(bytecodeArray);

    return true;
}

#if HADRON_PIPELINE_VALIDATE
bool Pipeline::validateFrame(ThreadContext* context, const Frame* frame, const parse::BlockNode* blockNode,
        const Lexer* lexer ) {
    int32_t argumentOrderArraySize = frame->argumentOrder.size();
    int32_t argumentDefaultsArraySize = frame->argumentDefaults.size();
    if (argumentOrderArraySize != argumentDefaultsArraySize) {
        SPDLOG_ERROR("Frame has mismatched argument order and defaults array sizes of {} and {} respectively",
            argumentOrderArraySize, argumentDefaultsArraySize);
        return false;
    }

    if (argumentOrderArraySize < 1) {
        SPDLOG_ERROR("First argument to Frame absent.");
        return false;
    }

    auto argName = frame->argumentOrder.at(0);
    if (argName.hash() != kThisHash) {
        SPDLOG_ERROR("First argument to Frame '{}' {:16x} not 'this' {:16x}", argName.view(context), argName.hash(),
                kThisHash);
        return false;
    }

    // Check argument count and ordering against frame.
    int32_t numberOfArguments = 1;
    if (blockNode->arguments && blockNode->arguments->varList) {
        const hadron::parse::VarDefNode* varDef = blockNode->arguments->varList->definitions.get();
        while (varDef) {
            auto nameHash = hadron::hash(lexer->tokens()[varDef->tokenIndex].range);
            if (argumentOrderArraySize < numberOfArguments) {
                SPDLOG_ERROR("Missing argument number {} named {} from Frame", numberOfArguments,
                        lexer->tokens()[varDef->tokenIndex].range);
                return false;
            }
            argName = frame->argumentOrder.at(numberOfArguments);
            if (argName.hash() != nameHash) {
                SPDLOG_ERROR("Mismatched hash for argument number {} named {}", numberOfArguments,
                        lexer->tokens()[varDef->tokenIndex].range);
                return false;
            }
            ++numberOfArguments;
            varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(varDef->next.get());
        }
        if (blockNode->arguments->varArgsNameIndex) {
            auto nameHash = hadron::hash(lexer->tokens()[blockNode->arguments->varArgsNameIndex.value()].range);
            if (argumentOrderArraySize < numberOfArguments) {
                SPDLOG_ERROR("Missing varArgs argument number {} named {} from Frame", numberOfArguments,
                        lexer->tokens()[blockNode->arguments->varArgsNameIndex.value()].range);
                return false;
            }
            argName = frame->argumentOrder.at(numberOfArguments);
            if (argName.hash() != nameHash) {
                SPDLOG_ERROR("Mismatched hash for varArgs number {} named {}", numberOfArguments,
                        lexer->tokens()[blockNode->arguments->varArgsNameIndex.value()].range);
                return false;
            }
            ++numberOfArguments;
        }
    }
    if (argumentOrderArraySize != numberOfArguments) {
        SPDLOG_ERROR("Mismatched argument count in Frame, parse tree has {}, Frame has {}", numberOfArguments,
            argumentOrderArraySize);
        return false;
    }

    std::unordered_set<int> blockNumbers;
    if (!validateSubScope(frame->rootScope.get(), nullptr, blockNumbers)) { return false; }

    if (frame->numberOfBlocks != static_cast<int>(blockNumbers.size())) {
        SPDLOG_ERROR("Base frame number of blocks {} mismatches counted amount of {}", frame->numberOfBlocks,
                blockNumbers.size());
        return false;
    }
    // There should be at least one Block.
    if (blockNumbers.size() < 1) {
        SPDLOG_ERROR("Base frame has no blocks");
        return false;
    }
    return true;
}

bool Pipeline::validateSubScope(const Scope* scope, const Scope* parent, std::unordered_set<int>& blockNumbers) {
    if (scope->parent != parent) {
        SPDLOG_ERROR("Scope parent mismatch");
        return false;
    }
    for (const auto& block : scope->blocks) {
        // Block must have a reference back to the correct owning frame.
        if (block->scope != scope) {
            SPDLOG_ERROR("Block frame mismatch");
            return false;
        }
        // Block numbers must be unique.
        if (blockNumbers.find(block->number) != blockNumbers.end()) {
            SPDLOG_ERROR("Non-unique block number {}", block->number);
            return false;
        }

        blockNumbers.emplace(block->number);
    }
    for (const auto& subScope : scope->subScopes) {
        if (!validateSubScope(subScope.get(), scope, blockNumbers)) { return false; }
    }

    return true;
}

// This validation is feeling very much like a "change detector" for the BlockSerializer class. However, the input
// requirements for the rest of the pipeline are specific so this serves as documentation and enforcement of those
// requiements. What the BlockSerializer does is somewhat "mechanical," too. It is likely there's a smarter way to test
// this code, so if the maintenence cost of this testing code becomes an undue burden please refactor. I expect the
// surfaces between BlockBuilder, BlockSerializer, and LifetimeAnalyzer to remain relatively stable, barring algorithm
// changes, so the hope is that the serializer changes relatively infrequently and therefore the maintenence cost is low
// compared to the increased confidence that the inputs to the rest of the compiler pipeline are valid.
bool Pipeline::validateSerializedBlock(const LinearBlock* linearBlock, size_t numberOfBlocks, size_t numberOfValues) {
    if (linearBlock->blockOrder.size() != numberOfBlocks || linearBlock->blockRanges.size() != numberOfBlocks) {
        SPDLOG_ERROR("Mismatch block count on serialization, expecting: {} blockOrder: {} blockRanges: {}",
                numberOfBlocks, linearBlock->blockOrder.size(), linearBlock->blockRanges.size());
        return false;
    }

    // The block order should see the ranges increasing with no gaps and covering all the instructions.
    size_t blockStart = 0;
    for (auto blockNumber : linearBlock->blockOrder) {
        if (blockNumber >= static_cast<int>(linearBlock->blockRanges.size()) || blockNumber < 0) {
            SPDLOG_ERROR("Block number {} out of range", blockNumber);
            return false;
        }
        auto range = linearBlock->blockRanges[blockNumber];
        if (range.first != blockStart) {
            SPDLOG_ERROR("Block not starting on correct line, expecting {} got {}", blockStart, range.first);
            return false;
        }
        // Every block needs to begin with a label.
        if (linearBlock->instructions[blockStart]->opcode != hadron::hir::Opcode::kLabel) {
            SPDLOG_ERROR("Block not starting with label at instruction {}", blockStart);
            return false;
        }

        // The label should have the correct blockNumber
        auto label = reinterpret_cast<const hir::LabelHIR*>(linearBlock->instructions[blockStart].get());
        if (label->blockNumber != blockNumber) {
            SPDLOG_ERROR("Block label number mistmatch");
            return false;
        }

        // Next block should start at the end of this block.
        blockStart = range.second;
    }
    if (linearBlock->instructions.size() != blockStart) {
        SPDLOG_ERROR("Final block doesn't end at end of instructions");
        return false;
    }

    // Value lifetimes should be the size of numberOfValues with one empty lifetime in each vector.
    if (linearBlock->valueLifetimes.size() != numberOfValues) {
        SPDLOG_ERROR("Mismatch value count on serialization, have {}, expected {}", linearBlock->valueLifetimes.size(),
                numberOfValues);
        return false;
    }

    for (size_t i = 0; i < numberOfValues; ++i) {
        if (linearBlock->valueLifetimes[i].size() != 1) {
            SPDLOG_ERROR("Expected 1 lifetime per value, value {} has {}", i, linearBlock->valueLifetimes[i].size());
            return false;
        }
        if (!linearBlock->valueLifetimes[i][0]->isEmpty()) {
            SPDLOG_ERROR("Non-empty value lifetime at value {}", i);
            return false;
        }
    }

    // The spill slot counter should remain at the default until register allocation.
    if (linearBlock->numberOfSpillSlots != 1) {
        SPDLOG_ERROR("Non-default value of {} for number of spill slots", linearBlock->numberOfSpillSlots);
        return false;
    }

    // Check for valid SSA form by ensuring all values are written only once, and they are written before they are read.
    std::unordered_set<uint32_t> values;
    for (size_t i = 0; i < linearBlock->instructions.size(); ++i) {
        const hir::HIR* hir = linearBlock->instructions[i].get();
        if (hir->opcode == hir::Opcode::kLabel) {
            auto label = reinterpret_cast<const hir::LabelHIR*>(hir);
            for (const auto& phi : label->phis) {
                if (!validateSsaHir(phi.get(), values)) { return false; }
            }
        }
        if (!validateSsaHir(hir, values)) { return false; }
    }

    return true;
}

bool Pipeline::validateSsaHir(const hir::HIR* hir, std::unordered_set<uint32_t>& values) {
    if (hir->value.isValid()) {
        if (values.count(hir->value.number)) {
            SPDLOG_ERROR("Duplicate definition of value {} in linear block.", hir->value.number);
            return false;
        }
        values.emplace(hir->value.number);
    }
    for (auto v : hir->reads) {
        if (!values.count(v.number)) {
            SPDLOG_ERROR("Value {} read before written.", v.number);
            return false;
        }
    }
    return true;
}

// There are some subtleties about block ranges, phis, and loops, which should be checked for correct behavior in
// individual test cases. The broad invariant this function checks is that all acesses of a value happen while it
// is live, and should also be in the usages set.
bool Pipeline::validateLifetimes(const LinearBlock* linearBlock) {
    for (size_t i = 0; i < linearBlock->valueLifetimes.size(); ++i) {
        if (linearBlock->valueLifetimes[i].size() != 1) {
            SPDLOG_ERROR("Expecting single element in value lifetimes arrays until register allocation");
            return false;
        }
    }

    std::vector<size_t> usageCounts(linearBlock->valueLifetimes.size(), 0);
    for (size_t i = 0; i < linearBlock->instructions.size(); ++i) {
        const auto hir = linearBlock->instructions[i].get();
        if (hir->value.isValid()) {
            if (!linearBlock->valueLifetimes[hir->value.number][0]->covers(i)) {
                SPDLOG_ERROR("value {} written outside of lifetime", hir->value.number);
                return false;
            }
            if (linearBlock->valueLifetimes[hir->value.number][0]->usages.count(i) != 1) {
                SPDLOG_ERROR("value {} written but not marked as used", hir->value.number);
                return false;
            }
            ++usageCounts[hir->value.number];
        }
        for (const auto& value : hir->reads) {
            if (!linearBlock->valueLifetimes[value.number][0]->covers(i)) {
                SPDLOG_ERROR("value {} read outside of lifetime at instruction {}", value.number, i);
                return false;
            }
            if (linearBlock->valueLifetimes[value.number][0]->usages.count(i) != 1) {
                SPDLOG_ERROR("value {} read without being marked as used at instruction {}", value.number, i);
                return false;
            }
            ++usageCounts[value.number];
        }
    }

    for (size_t i = 0; i < linearBlock->valueLifetimes.size(); ++i) {
        if (linearBlock->valueLifetimes[i][0]->valueNumber != i) {
            SPDLOG_ERROR("Value number mismatch at value {}", i);
            return false;
        }
        if (linearBlock->valueLifetimes[i][0]->usages.size() != usageCounts[i]) {
            SPDLOG_ERROR("Usage count mismatch on value {}", i);
            return false;
        }

    }

    return true;
}

// Check that at instruction |i| there is exactly one register allocated to |vReg|.
bool Pipeline::validateRegisterCoverage(const LinearBlock* linearBlock, size_t i, uint32_t vReg) {
    int valueCovered = 0;
    size_t reg = 0;
    for (const auto& lt : linearBlock->valueLifetimes[vReg]) {
        if (lt->isSpill) { continue; }
        if (lt->covers(i)) {
            if (lt->usages.count(i) != 1) {
                SPDLOG_ERROR("Value live but no usage at {}", i);
                return false;
            }
            ++valueCovered;
            reg = lt->registerNumber;
        }
    }
    if (valueCovered != 1) {
        SPDLOG_ERROR("Value {} not covered (or over-covered) at {}", vReg, i);
        return false;
    }
    if (reg >= m_numberOfRegisters) {
        SPDLOG_ERROR("Bad register number {} for value {} at instruction {}", reg, vReg, i);
        return false;
    }

    // Check the valueLocations map at the instruction to make sure it's accurate.
    auto iter = linearBlock->instructions[i]->valueLocations.find(vReg);
    if (iter == linearBlock->instructions[i]->valueLocations.end() || reg != static_cast<size_t>(iter->second)) {
        SPDLOG_ERROR("Value {} at register {} absent or different in map at instruction {}", vReg, reg, i);
        return false;
    }

    // Ensure no other values at this instruction are allocated to this same register.
    for (size_t j = 0; j < linearBlock->valueLifetimes.size(); ++j) {
        if (j == vReg) { continue; }
        for (const auto& lt : linearBlock->valueLifetimes[j]) {
            if (lt->isSpill) { continue; }
            if (lt->covers(i)) {
                if (lt->registerNumber == reg) {
                    SPDLOG_ERROR("Duplicate register allocation for register {}, values {} and {}, at instruction {}",
                            reg, vReg, j, i);
                    return false;
                }
            }
        }
    }

    return true;
}

bool Pipeline::validateAllocation(const hadron::LinearBlock* linearBlock) {
    // Value numbers should align across the valueLifetimes arrays.
    for (size_t i = 0; i < linearBlock->valueLifetimes.size(); ++i) {
        for (const auto& lt : linearBlock->valueLifetimes[i]) {
            if (lt->valueNumber != i) {
                SPDLOG_ERROR("Mismatch value number at {}", i);
                return false;
            }
        }
    }

    // Every usage of every virtual register should have a single physical register assigned.
    for (size_t i = 0; i < linearBlock->instructions.size(); ++i) {
        const auto* hir = linearBlock->instructions[i].get();
        if (hir->value.isValid()) {
            if (!validateRegisterCoverage(linearBlock, i, hir->value.number)) { return false; }
        }
        for (const auto& value : hir->reads) {
            if (!validateRegisterCoverage(linearBlock, i, value.number)) { return false; }
        }
    }

    return true;
}

bool Pipeline::validateResolution(const LinearBlock*) {
    // TODO: Could go through and look at boundaries for each block, validating that the expectations of where values
    // are have been met from each predecessor block.
    return true;
}

bool Pipeline::validateEmission(const LinearBlock*, library::Int8Array) {
    return true;
}

#endif // HADRON_PIPELINE_VALIDATE

} // namespace hadron