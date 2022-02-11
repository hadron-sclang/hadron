#include "hadron/Pipeline.hpp"

#include "hadron/Arch.hpp"
#include "hadron/AST.hpp"
#include "hadron/ASTBuilder.hpp"
#include "hadron/Block.hpp"
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
#include "hadron/LinearBlock.hpp"
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
    auto linearBlock = serializer.serialize(frame.get());
    if (!linearBlock) { return false; }
#if HADRON_PIPELINE_VALIDATE
    if (!validateSerializedBlock(linearBlock.get(), numberOfBlocks)) { return false; }
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
    for (const auto& lir : linearBlock->instructions) {
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
bool Pipeline::validateFrame(ThreadContext* /* context */, const Frame* frame, const ast::BlockAST* /* blockAST */) {
    int32_t argumentOrderArraySize = frame->argumentOrder.size();
    int32_t argumentDefaultsArraySize = frame->argumentDefaults.size();
    if (argumentOrderArraySize != argumentDefaultsArraySize) {
        SPDLOG_ERROR("Frame has mismatched argument order and defaults array sizes of {} and {} respectively",
            argumentOrderArraySize, argumentDefaultsArraySize);
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
bool Pipeline::validateSerializedBlock(const LinearBlock* linearBlock, size_t numberOfBlocks) {
    if (linearBlock->blockOrder.size() != numberOfBlocks || linearBlock->blockRanges.size() != numberOfBlocks) {
        SPDLOG_ERROR("Mismatch block count on serialization, expecting: {} blockOrder: {} blockRanges: {}",
                numberOfBlocks, linearBlock->blockOrder.size(), linearBlock->blockRanges.size());
        return false;
    }

    // Check for valid SSA form by ensuring all values are written only once, and they are written before they are read.
    std::unordered_set<lir::VReg> values;
    for (const auto& lir : linearBlock->instructions) {
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
            SPDLOG_ERROR("Duplicate definition of value {} in linear block.", lir->value);
            return false;
        }
        values.emplace(lir->value);
    }
    for (auto v : lir->reads) {
        if (!values.count(v)) {
            SPDLOG_ERROR("Value {} read before written.", v);
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
        if (linearBlock->lineNumbers[blockStart]->opcode != hadron::lir::Opcode::kLabel) {
            SPDLOG_ERROR("Block not starting with label at instruction {}", blockStart);
            return false;
        }

        // The label should have the correct blockNumber
        auto label = reinterpret_cast<const lir::LabelLIR*>(linearBlock->lineNumbers[blockStart]);
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

    // The spill slot counter should remain at the default until register allocation.
    if (linearBlock->numberOfSpillSlots != 1) {
        SPDLOG_ERROR("Non-default value of {} for number of spill slots", linearBlock->numberOfSpillSlots);
        return false;
    }


    std::vector<size_t> usageCounts(linearBlock->valueLifetimes.size(), 0);
    for (size_t i = 0; i < linearBlock->lineNumbers.size(); ++i) {
        const auto lir = linearBlock->lineNumbers[i];
        if (lir->value != lir::kInvalidVReg) {
            if (!linearBlock->valueLifetimes[lir->value][0]->covers(i)) {
                SPDLOG_ERROR("value {} written outside of lifetime", lir->value);
                return false;
            }
            if (linearBlock->valueLifetimes[lir->value][0]->usages.count(i) != 1) {
                SPDLOG_ERROR("value {} written but not marked as used", lir->value);
                return false;
            }
            ++usageCounts[lir->value];
        }
        for (auto value : lir->reads) {
            if (!linearBlock->valueLifetimes[value][0]->covers(i)) {
                SPDLOG_ERROR("value {} read outside of lifetime at instruction {}", value, i);
                return false;
            }
            if (linearBlock->valueLifetimes[value][0]->usages.count(i) != 1) {
                SPDLOG_ERROR("value {} read without being marked as used at instruction {}", value, i);
                return false;
            }
            ++usageCounts[value];
        }
    }

    for (int32_t i = 0; i < static_cast<int32_t>(linearBlock->valueLifetimes.size()); ++i) {
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
    auto iter = linearBlock->lineNumbers[i]->valueLocations.find(vReg);
    if (iter == linearBlock->lineNumbers[i]->valueLocations.end() || reg != static_cast<size_t>(iter->second)) {
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
    for (int32_t i = 0; i < static_cast<int32_t>(linearBlock->valueLifetimes.size()); ++i) {
        for (const auto& lt : linearBlock->valueLifetimes[i]) {
            if (lt->valueNumber != i) {
                SPDLOG_ERROR("Mismatch value number at {}", i);
                return false;
            }
        }
    }

    // Every usage of every virtual register should have a single physical register assigned.
    for (size_t i = 0; i < linearBlock->instructions.size(); ++i) {
        const auto* lir = linearBlock->lineNumbers[i];
        if (lir->value != lir::kInvalidVReg) {
            if (!validateRegisterCoverage(linearBlock, i, lir->value)) { return false; }
        }
        for (const auto& value : lir->reads) {
            if (!validateRegisterCoverage(linearBlock, i, value)) { return false; }
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