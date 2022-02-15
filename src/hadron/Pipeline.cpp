#include "hadron/Pipeline.hpp"

#include "hadron/Arch.hpp"
#include "hadron/AST.hpp"
#include "hadron/ASTBuilder.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/Emitter.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Hash.hpp"
#include "hadron/Heap.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/lir/LabelLIR.hpp"
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

    ASTBuilder astBuilder(m_errorReporter);
    auto blockAST = astBuilder.buildBlock(context, &lexer, reinterpret_cast<const parse::BlockNode*>(parser.root()));

    return compileBlock(context, blockAST.get());
}

library::FunctionDef Pipeline::compileBlock(ThreadContext* context, const ast::BlockAST* blockAST) {
    auto functionDef = library::FunctionDef::alloc(context);
    buildBlock(context, blockAST, functionDef);
    return functionDef;
}

library::Method Pipeline::compileMethod(ThreadContext* context, const library::Class /* classDef */,
        const ast::BlockAST* blockAST) {
    auto method = library::Method::alloc(context);
    buildBlock(context, blockAST, library::FunctionDef::wrapUnsafe(method.instance()));
    return method;
}

#if HADRON_PIPELINE_VALIDATE
bool Pipeline::afterBlockBuilder(const Frame*, const ast::BlockAST*) { return true; }
bool Pipeline::afterBlockSerializer(const LinearFrame*) { return true; }
bool Pipeline::afterLifetimeAnalyzer(const LinearFrame*) { return true; }
bool Pipeline::afterRegisterAllocator(const LinearFrame*) { return true; }
bool Pipeline::afterResolver(const LinearFrame*) { return true; }
bool Pipeline::afterEmitter(const LinearFrame*, library::Int8Array) { return true; }
#endif // HADRON_PIPELINE_VALIDATE

void Pipeline::setDefaults() {
    m_numberOfRegisters = kNumberOfPhysicalRegisters;
    m_jitToVirtualMachine = false;
}

bool Pipeline::buildBlock(ThreadContext* context, const ast::BlockAST* blockAST, library::FunctionDef functionDef) {
    hadron::BlockBuilder builder(m_errorReporter);
    auto frame = builder.buildFrame(context, blockAST);
    if (!frame) { return false; }
#if HADRON_PIPELINE_VALIDATE
    if (!validateFrame(context, frame.get(), blockAST)) { return false; }
    if (!afterBlockBuilder(frame.get(), blockAST)) { return false; }
    size_t numberOfBlocks = static_cast<size_t>(frame->numberOfBlocks);
#endif // HADRON_PIPELINE_VALIDATE

    functionDef.setArgNames(frame->argumentOrder);
    functionDef.setPrototypeFrame(frame->argumentDefaults);

    BlockSerializer serializer;
    auto linearFrame = serializer.serialize(frame.get());
    if (!linearFrame) { return false; }
#if HADRON_PIPELINE_VALIDATE
    if (!validateLinearFrame(linearFrame.get(), numberOfBlocks)) { return false; }
    if (!afterBlockSerializer(linearFrame.get())) { return false; }
#endif // HADRON_PIPELINE_VALIDATE

    LifetimeAnalyzer analyzer;
    analyzer.buildLifetimes(linearFrame.get());
#if HADRON_PIPELINE_VALIDATE
    if (!validateLifetimes(linearFrame.get())) { return false; }
    if (!afterLifetimeAnalyzer(linearFrame.get())) { return false; }
#endif // HADRON_PIPELINE_VALIDATE

    RegisterAllocator allocator(m_numberOfRegisters);
    allocator.allocateRegisters(linearFrame.get());
#if HADRON_PIPELINE_VALIDATE
    if (!validateAllocation(linearFrame.get())) { return false; }
    if (!afterRegisterAllocator(linearFrame.get())) { return false; }
#endif // HADRON_PIPELINE_VALIDATE

    Resolver resolver;
    resolver.resolve(linearFrame.get());
#if HADRON_PIPELINE_VALIDATE
    if (!validateResolution(linearFrame.get())) { return false; }
    if (!afterResolver(linearFrame.get())) { return false; }
#endif // HADRON_PIPELINE_VALIDATE

    size_t jitMaxSize = sizeof(library::Int8Array);
    for (const auto& lir : linearFrame->instructions) {
        jitMaxSize += 16 + (16 * lir->moves.size());
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
    emitter.emit(linearFrame.get(), jit.get());
    assert(!jit->hasJITBufferOverflow());
    size_t finalSize = 0;
    jit->end(&finalSize);
    bytecodeArray.resize(context, finalSize);

#if HADRON_PIPELINE_VALIDATE
    if (!validateEmission(linearFrame.get(), bytecodeArray)) { return false; }
    if (!afterEmitter(linearFrame.get(), bytecodeArray)) { return false; }
#endif

    functionDef.setCode(bytecodeArray);

    return true;
}

#if HADRON_PIPELINE_VALIDATE
bool Pipeline::validateFrame(ThreadContext* /* context */, const Frame* frame, const ast::BlockAST* /* blockAST */) {
    int32_t argumentOrderArraySize = frame->argumentOrder.size();
    int32_t argumentDefaultsArraySize = frame->argumentDefaults.size();
    if (argumentOrderArraySize != argumentDefaultsArraySize) {
        SPDLOG_ERROR("Frame has mismatched argument order and defaults array sizes of {} and {} respectively",
            argumentOrderArraySize, argumentDefaultsArraySize);
        return false;
    }

    std::unordered_set<Block::ID> blockIds;
    std::unordered_set<hir::NVID> valueIds;
    if (!validateSubScope(frame->rootScope.get(), nullptr, blockIds, valueIds)) { return false; }

    if (frame->numberOfBlocks != static_cast<int>(blockIds.size())) {
        SPDLOG_ERROR("Base frame number of blocks {} mismatches counted amount of {}", frame->numberOfBlocks,
                blockIds.size());
        return false;
    }
    // There should be at least one Block.
    if (blockIds.size() < 1) {
        SPDLOG_ERROR("Base frame has no blocks");
        return false;
    }
    return true;
}

bool Pipeline::validateSubScope(const Scope* scope, const Scope* parent, std::unordered_set<Block::ID>& blockIds,
        std::unordered_set<hir::NVID>& valueIds) {
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
        // Block ids must be unique.
        if (blockIds.find(block->id) != blockIds.end()) {
            SPDLOG_ERROR("Non-unique block number {}", block->id);
            return false;
        }

        for (const auto& phi : block->phis) {
            if (valueIds.count(phi->value.id)) {
                SPDLOG_ERROR("Duplicate NVID {} found in phi in block {}", phi->value.id, block->id);
                return false;
            }
            if (scope->frame->values[phi->value.id] != phi.get()) {
                SPDLOG_ERROR("Mismatch in phi between value id and pointer for NVID {}", phi->value.id);
                return false;
            }
            valueIds.emplace(phi->value.id);
        }

        for (const auto& hir : block->statements) {
            if (hir->value.id != hir::kInvalidNVID) {
                if (valueIds.count(hir->value.id)) {
                    SPDLOG_ERROR("Duplicate NVID {} found for hir in block {}", hir->value.id, block->id);
                    return false;
                }
                if (scope->frame->values[hir->value.id] != hir.get()) {
                    SPDLOG_ERROR("Mismatch between value id and pointer for NVID {}", hir->value.id);
                    return false;
                }
                valueIds.emplace(hir->value.id);
            }
        }

        blockIds.emplace(block->id);
    }
    for (const auto& subScope : scope->subScopes) {
        if (!validateSubScope(subScope.get(), scope, blockIds, valueIds)) { return false; }
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
bool Pipeline::validateLinearFrame(const LinearFrame* linearFrame, size_t numberOfBlocks) {
    if (linearFrame->blockOrder.size() != numberOfBlocks || linearFrame->blockLabels.size() != numberOfBlocks) {
        SPDLOG_ERROR("Mismatch block count on serialization, expecting: {} blockOrder: {} blockLabels: {}",
                numberOfBlocks, linearFrame->blockOrder.size(), linearFrame->blockLabels.size());
        return false;
    }

    // Check for valid SSA form by ensuring all values are written only once, and they are written before they are read.
    std::unordered_set<lir::VReg> values;
    for (const auto& lir : linearFrame->instructions) {
        if (lir->opcode == lir::Opcode::kLabel) {
            auto label = reinterpret_cast<const lir::LabelLIR*>(lir.get());
            for (const auto& phi : label->phis) {
                if (!validateSsaLir(phi.get(), values)) { return false; }
            }
        }
        if (!validateSsaLir(lir.get(), values)) { return false; }
    }

    return true;
}

bool Pipeline::validateSsaLir(const lir::LIR* lir, std::unordered_set<lir::VReg>& values) {
    if (lir->value != lir::kInvalidVReg) {
        if (values.count(lir->value)) {
            SPDLOG_ERROR("Duplicate definition of vReg {} in linear block.", lir->value);
            return false;
        }
        values.emplace(lir->value);
    }
    for (auto v : lir->reads) {
        if (v == lir::kInvalidVReg) {
            SPDLOG_ERROR("Invalid vReg value in reads set.");
            return false;
        }
        if (!values.count(v)) {
            SPDLOG_ERROR("LIR vReg {} read before written.", v);
            return false;
        }
    }
    return true;
}

// There are some subtleties about block ranges, phis, and loops, which should be checked for correct behavior in
// individual test cases. The broad invariant this function checks is that all acesses of a value happen while it
// is live, and should also be in the usages set.
bool Pipeline::validateLifetimes(const LinearFrame* linearFrame) {
    for (size_t i = 0; i < linearFrame->valueLifetimes.size(); ++i) {
        if (linearFrame->valueLifetimes[i].size() != 1) {
            SPDLOG_ERROR("Expecting single element in value lifetimes arrays until register allocation");
            return false;
        }
    }

    // The block order should see the ranges increasing with no gaps and covering all the instructions.
    size_t blockStart = 0;
    for (auto labelId : linearFrame->blockOrder) {
        if (labelId >= static_cast<Block::ID>(linearFrame->blockRanges.size()) || labelId < 0) {
            SPDLOG_ERROR("Block number {} out of range", labelId);
            return false;
        }
        auto range = linearFrame->blockRanges[labelId];
        if (range.first != blockStart) {
            SPDLOG_ERROR("Block not starting on correct line, expecting {} got {}", blockStart, range.first);
            return false;
        }
        // Every block needs to begin with a label.
        if (linearFrame->lineNumbers[blockStart]->opcode != hadron::lir::Opcode::kLabel) {
            SPDLOG_ERROR("Block not starting with label at instruction {}", blockStart);
            return false;
        }

        // The label should have the correct blockNumber
        auto label = reinterpret_cast<const lir::LabelLIR*>(linearFrame->lineNumbers[blockStart]);
        if (label->id != labelId) {
            SPDLOG_ERROR("Block label number mistmatch");
            return false;
        }

        // Next block should start at the end of this block.
        blockStart = range.second;
    }
    if (linearFrame->instructions.size() != blockStart) {
        SPDLOG_ERROR("Final block doesn't end at end of instructions");
        return false;
    }

    // The spill slot counter should remain at the default until register allocation.
    if (linearFrame->numberOfSpillSlots != 1) {
        SPDLOG_ERROR("Non-default value of {} for number of spill slots", linearFrame->numberOfSpillSlots);
        return false;
    }

    std::vector<size_t> usageCounts(linearFrame->valueLifetimes.size(), 0);
    for (size_t i = 0; i < linearFrame->lineNumbers.size(); ++i) {
        const auto lir = linearFrame->lineNumbers[i];
        if (lir->value != lir::kInvalidVReg) {
            if (!linearFrame->valueLifetimes[lir->value][0]->covers(i)) {
                SPDLOG_ERROR("value {} written outside of lifetime", lir->value);
                return false;
            }
            if (linearFrame->valueLifetimes[lir->value][0]->usages.count(i) != 1) {
                SPDLOG_ERROR("value {} written but not marked as used", lir->value);
                return false;
            }
            ++usageCounts[lir->value];
        }
        for (auto value : lir->reads) {
            if (!linearFrame->valueLifetimes[value][0]->covers(i)) {
                SPDLOG_ERROR("value {} read outside of lifetime at instruction {}", value, i);
                return false;
            }
            if (linearFrame->valueLifetimes[value][0]->usages.count(i) != 1) {
                SPDLOG_ERROR("value {} read without being marked as used at instruction {}", value, i);
                return false;
            }
            ++usageCounts[value];
        }
    }

    for (int32_t i = 0; i < static_cast<int32_t>(linearFrame->valueLifetimes.size()); ++i) {
        if (linearFrame->valueLifetimes[i][0]->valueNumber != i) {
            SPDLOG_ERROR("Value number mismatch at value {}", i);
            return false;
        }
        if (linearFrame->valueLifetimes[i][0]->usages.size() != usageCounts[i]) {
            SPDLOG_ERROR("Usage count mismatch on value {}", i);
            return false;
        }

    }

    return true;
}

// Check that at instruction |i| there is exactly one register allocated to |vReg|.
bool Pipeline::validateRegisterCoverage(const LinearFrame* linearFrame, size_t i, uint32_t vReg) {
    int valueCovered = 0;
    size_t reg = 0;
    for (const auto& lt : linearFrame->valueLifetimes[vReg]) {
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
    auto iter = linearFrame->lineNumbers[i]->locations.find(vReg);
    if (iter == linearFrame->lineNumbers[i]->locations.end() || reg != static_cast<size_t>(iter->second)) {
        SPDLOG_ERROR("Value {} at register {} absent or different in map at instruction {}", vReg, reg, i);
        return false;
    }

    // Ensure no other values at this instruction are allocated to this same register.
    for (size_t j = 0; j < linearFrame->valueLifetimes.size(); ++j) {
        if (j == vReg) { continue; }
        for (const auto& lt : linearFrame->valueLifetimes[j]) {
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

bool Pipeline::validateAllocation(const hadron::LinearFrame* linearFrame) {
    // Value numbers should align across the valueLifetimes arrays.
    for (int32_t i = 0; i < static_cast<int32_t>(linearFrame->valueLifetimes.size()); ++i) {
        for (const auto& lt : linearFrame->valueLifetimes[i]) {
            if (lt->valueNumber != i) {
                SPDLOG_ERROR("Mismatch value number at {}", i);
                return false;
            }
        }
    }

    // Every usage of every virtual register should have a single physical register assigned.
    for (size_t i = 0; i < linearFrame->instructions.size(); ++i) {
        const auto* lir = linearFrame->lineNumbers[i];
        if (lir->value != lir::kInvalidVReg) {
            if (!validateRegisterCoverage(linearFrame, i, lir->value)) { return false; }
        }
        for (const auto& value : lir->reads) {
            if (!validateRegisterCoverage(linearFrame, i, value)) { return false; }
        }
    }

    return true;
}

bool Pipeline::validateResolution(const LinearFrame*) {
    // TODO: Could go through and look at boundaries for each block, validating that the expectations of where values
    // are have been met from each predecessor block.
    return true;
}

bool Pipeline::validateEmission(const LinearFrame*, library::Int8Array) {
    return true;
}

#endif // HADRON_PIPELINE_VALIDATE

} // namespace hadron