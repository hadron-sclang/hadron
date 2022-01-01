#include "hadron/Pipeline.hpp"

#include "hadron/Block.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
//#include "hadron/Emitter.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Hash.hpp"
#include "hadron/HIR.hpp"
//#include "hadron/Heap.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/Parser.hpp"
#include "hadron/RegisterAllocator.hpp"
#include "hadron/Resolver.hpp"
//#include "hadron/SourceFile.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

Pipeline::Pipeline(): m_errorReporter(std::make_shared<ErrorReporter>()) { setDefaults(); }

Pipeline::Pipeline(std::shared_ptr<ErrorReporter> errorReporter): m_errorReporter(errorReporter) { setDefaults(); }

Pipeline::~Pipeline() {}

std::unique_ptr<LinearBlock> Pipeline::compileBlock(std::string_view code) {
    Lexer lexer(code, m_errorReporter);
    if (!lexer.lex()) { return nullptr; }

    Parser parser(&lexer, m_errorReporter);
    if (!parser.parse()) { return nullptr; }

    assert(parser.root()->nodeType == parse::NodeType::kBlock);

    return buildBlock(reinterpret_cast<const parse::BlockNode*>(parser.root()), &lexer);
}

#if HADRON_PIPELINE_VALIDATE
bool Pipeline::afterBlockBuilder(const Frame*, const parse::BlockNode*, const Lexer*) { return true; }
bool Pipeline::afterBlockSerializer(const LinearBlock*) { return true; }
bool Pipeline::afterLifetimeAnalyzer(const LinearBlock*) { return true; }
bool Pipeline::afterRegisterAllocator(const LinearBlock*) { return true; }
bool Pipeline::afterResolver(const LinearBlock*) { return true; }
#endif // HADRON_PIPELINE_VALIDATE

void Pipeline::setDefaults() {
    m_numberOfRegisters = LighteningJIT::physicalRegisterCount();
}

std::unique_ptr<LinearBlock> Pipeline::buildBlock(const parse::BlockNode* blockNode, const Lexer* lexer) {
    hadron::BlockBuilder builder(lexer, m_errorReporter);
    auto frame = builder.buildFrame(blockNode);
    if (!frame) { return nullptr; }
#if HADRON_PIPELINE_VALIDATE
    if (!validateFrame(frame.get(), blockNode, lexer)) { return nullptr; }
    if (!afterBlockBuilder(frame.get(), blockNode, lexer)) { return nullptr; }
    size_t numberOfBlocks = static_cast<size_t>(frame->numberOfBlocks);
    size_t numberOfValues = frame->numberOfValues;
#endif // HADRON_PIPELINE_VALIDATE

    BlockSerializer serializer;
    auto linearBlock = serializer.serialize(std::move(frame), m_numberOfRegisters);
    if (!linearBlock) { return nullptr; }
#if HADRON_PIPELINE_VALIDATE
    if (!validateSerializedBlock(linearBlock.get(), numberOfBlocks, numberOfValues)) { return nullptr; }
    if (!afterBlockSerializer(linearBlock.get())) { return nullptr; }
#endif // HADRON_PIPELINE_VALIDATE

    LifetimeAnalyzer analyzer;
    analyzer.buildLifetimes(linearBlock.get());
#if HADRON_PIPELINE_VALIDATE
    if (!validateLifetimes(linearBlock.get())) { return nullptr; }
    if (!afterLifetimeAnalyzer(linearBlock.get())) { return nullptr; }
#endif // HADRON_PIPELINE_VALIDATE

    RegisterAllocator allocator;
    allocator.allocateRegisters(linearBlock.get());
#if HADRON_PIPELINE_VALIDATE
    if (!validateAllocation(linearBlock.get())) { return nullptr; }
    if (!afterRegisterAllocator(linearBlock.get())) { return nullptr; }
#endif // HADRON_PIPELINE_VALIDATE

    Resolver resolver;
    resolver.resolve(linearBlock.get());
#if HADRON_PIPELINE_VALIDATE
    if (!validateResolution(linearBlock.get())) { return nullptr; }
    if (!afterResolver(linearBlock.get())) { return nullptr; }
#endif // HADRON_PIPELINE_VALIDATE

    return linearBlock;
}

#if HADRON_PIPELINE_VALIDATE
bool Pipeline::validateFrame(const Frame* frame, const parse::BlockNode* blockNode, const Lexer* lexer) {
    if (frame->argumentOrder.size() < 1 || frame->argumentOrder[0] != kThisHash) {
        SPDLOG_ERROR("First argument to Frame either absent or not 'this'");
        return false;
    }

    // Check argument count and ordering against frame.
    size_t numberOfArguments = 1;
    if (blockNode->arguments && blockNode->arguments->varList) {
        const hadron::parse::VarDefNode* varDef = blockNode->arguments->varList->definitions.get();
        while (varDef) {
            auto nameHash = hadron::hash(lexer->tokens()[varDef->tokenIndex].range);
            if (frame->argumentOrder.size() < numberOfArguments) {
                SPDLOG_ERROR("Missing argument number {} named {} from Frame", numberOfArguments,
                        lexer->tokens()[varDef->tokenIndex].range);
                return false;
            }
            if (frame->argumentOrder[numberOfArguments] != nameHash) {
                SPDLOG_ERROR("Mismatched hash for argument number {} named {}", numberOfArguments,
                        lexer->tokens()[varDef->tokenIndex].range);
                return false;
            }
            ++numberOfArguments;
            varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(varDef->next.get());
        }
    }
    if (frame->argumentOrder.size() != numberOfArguments) {
        SPDLOG_ERROR("Mismatched argument count in Frame, parse tree has {}, Frame has {}", numberOfArguments,
            frame->argumentOrder.size());
        return false;
    }

    std::unordered_map<uint32_t, uint32_t> values;
    std::unordered_set<int> blockNumbers;
    if (!validateSubFrame(frame, nullptr, values, blockNumbers)) { return false; }

    if (frame->numberOfValues != values.size()) {
        SPDLOG_ERROR("Base frame number of values {} mismatches counted amount of {}", frame->numberOfValues,
            values.size());
        return false;
    }
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

bool Pipeline::validateSubFrame(const Frame* frame, const Frame* parent,
        std::unordered_map<uint32_t, uint32_t>& values, std::unordered_set<int>& blockNumbers) {
    if (frame->parent != parent) {
        SPDLOG_ERROR("Frame parent mismatch");
        return false;
    }
    for (const auto& block : frame->blocks) {
        // Block must have a reference back to the correct owning frame.
        if (block->frame != frame) {
            SPDLOG_ERROR("Block frame mismatch");
            return false;
        }
        // Block numbers must be unique.
        if (blockNumbers.find(block->number) != blockNumbers.end()) {
            SPDLOG_ERROR("Non-unique block number {}", block->number);
            return false;
        }

        blockNumbers.emplace(block->number);

        for (const auto& phi : block->phis) {
            if (!validateFrameHIR(phi.get(), values, block.get())) { return false; }
        }
        for (const auto& hir : block->statements) {
            if (!validateFrameHIR(hir.get(), values, block.get())) { return false; }
        }
    }
    for (const auto& subFrame : frame->subFrames) {
        if (!validateSubFrame(subFrame.get(), frame, values, blockNumbers)) { return false; }
    }

    return true;
}

bool Pipeline::validateFrameHIR(const hir::HIR* hir, std::unordered_map<uint32_t, uint32_t>& values,
        const Block* block) {
    // Unique values should only be written to once.
    if (values.find(hir->value.number) != values.end()) {
        SPDLOG_ERROR("Value number {} written more than once", hir->value.number);
        return false;
    }
    for (const auto& read : hir->reads) {
        // Read values should all exist already and with the same type ornamentation.
        auto value = values.find(read.number);
        if (value == values.end()) {
            SPDLOG_ERROR("Value number {} read before written", read.number);
            return false;
        }
        if (read.typeFlags != value->second) {
            SPDLOG_ERROR("Inconsistent type flags for value number {}", read.number);
            return false;
        }
        // Read values should also exist in the local value map.
        if (block->localValues.find(read) == block->localValues.end()) {
            SPDLOG_ERROR("Value number {} absent from local map", read.number);
            return false;
        }
    }

    values.emplace(std::make_pair(hir->value.number, hir->value.typeFlags));
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
        SPDLOG_ERROR("Mismatch block count on serialization, expecting {}", numberOfBlocks);
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

        // TODO: How to check from here that the predecessors are all the dominators?

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
        if (!linearBlock->valueLifetimes[i][0].isEmpty()) {
            SPDLOG_ERROR("Non-empty value lifetime at value {}", i);
            return false;
        }
    }

    // Make a list of the indices of DispatchHIR instructions, as we expect registers to be reserved for those.
    std::vector<size_t> dispatchHIRIndices;
    for (size_t i = 0; i < linearBlock->instructions.size(); ++i) {
        if (linearBlock->instructions[i]->opcode == hadron::hir::Opcode::kDispatchCall) {
            dispatchHIRIndices.emplace_back(i);
        }
    }

    // There should be a single lifetime for each register, and it should be reserved for each Dispatch opcode as well
    // as the first instruction past the end of the program.
    if (linearBlock->registerLifetimes.size() != m_numberOfRegisters) {
        SPDLOG_ERROR("Register lifetime count mismatch, got {}, expected {}", linearBlock->registerLifetimes.size(),
                m_numberOfRegisters);
        return false;
    }
    for (size_t reg = 0; reg < m_numberOfRegisters; ++reg) {
        if (linearBlock->registerLifetimes[reg].size() != 1) {
            SPDLOG_ERROR("Register number {} has {} lifetimes, expected 1", linearBlock->registerLifetimes[reg].size());
            return false;
        }
        if (linearBlock->registerLifetimes[reg][0].registerNumber != reg) {
            SPDLOG_ERROR("Register number mismatch on lifetime for register {}", reg);
            return false;
        }
        if (linearBlock->registerLifetimes[reg][0].ranges.size() != dispatchHIRIndices.size() + 1) {
            SPDLOG_ERROR("Register lifetime range count not matching expected range count");
            return false;
        }
        auto rangeIter = linearBlock->registerLifetimes[reg][0].ranges.begin();
        for (auto index : dispatchHIRIndices) {
            if (rangeIter->from != index || rangeIter->to != index + 1) {
                SPDLOG_ERROR("Register range not covering DispatchHIR");
                return false;
            }
            ++rangeIter;
        }
        if (rangeIter->from != linearBlock->instructions.size() ||
            rangeIter->to != linearBlock->instructions.size() + 1) {
            SPDLOG_ERROR("Final register reservation not at end of instructions");
            return false;
        }
    }

    // Spill lifetimes are empty until we finish register allocation.
    if (linearBlock->spillLifetimes.size() != 0) {
        SPDLOG_ERROR("Spill lifetimes should be empty until register allocation");
        return false;
    }

    // The spill slot counter should remain at the default until register allocation.
    if (linearBlock->numberOfSpillSlots != 1) {
        SPDLOG_ERROR("Non-default value of {} for number of spill slots", linearBlock->numberOfSpillSlots);
        return false;
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
            if (!linearBlock->valueLifetimes[hir->value.number][0].covers(i)) {
                SPDLOG_ERROR("value {} written outside of lifetime", hir->value.number);
                return false;
            }
            if (linearBlock->valueLifetimes[hir->value.number][0].usages.count(i) != 1) {
                SPDLOG_ERROR("value {} written but not marked as used", hir->value.number);
                return false;
            }
            ++usageCounts[hir->value.number];
        }
        for (const auto& value : hir->reads) {
            if (!linearBlock->valueLifetimes[value.number][0].covers(i)) {
                SPDLOG_ERROR("value {} read outside of lifetime", value.number);
                return false;
            }
            if (linearBlock->valueLifetimes[value.number][0].usages.count(i) != 1) {
                SPDLOG_ERROR("value {} read without being marked as used", value.number);
                return false;
            }
            ++usageCounts[value.number];
        }
    }

    for (size_t i = 0; i < linearBlock->valueLifetimes.size(); ++i) {
        if (linearBlock->valueLifetimes[i][0].valueNumber != i) {
            SPDLOG_ERROR("Value number mismatch at value {}", i);
            return false;
        }
        if (linearBlock->valueLifetimes[i][0].usages.size() != usageCounts[i]) {
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
        if (lt.covers(i)) {
            if (lt.usages.count(i) != 1) {
                SPDLOG_ERROR("Value live but no usage at {}", i);
                return false;
            }
            ++valueCovered;
            reg = lt.registerNumber;
        }
    }
    if (valueCovered != 1) {
        SPDLOG_ERROR("Value {} not covered (or over-covered) at {}", vReg, i);
        return false;
    }

    int regCovered = 0;
    for (const auto& lt : linearBlock->registerLifetimes[reg]) {
        if (lt.covers(i)) {
            if (lt.usages.count(i) != 1) {
                SPDLOG_ERROR("Register {} live but not used for value {} at {}", reg, vReg, i);
                return false;
            }
            ++regCovered;
        }
    }
    if (regCovered != 1) {
        SPDLOG_ERROR("Register {} missing or over coverage for value {} at {}", reg, vReg, i);
        return false;
    }

    return true;
}

bool Pipeline::validateAllocation(const hadron::LinearBlock* linearBlock) {
    // Value numbers should align across the valueLifetimes arrays.
    for (size_t i = 0; i < linearBlock->valueLifetimes.size(); ++i) {
        for (const auto& lt : linearBlock->valueLifetimes[i]) {
            if (lt.valueNumber != i) {
                SPDLOG_ERROR("Mismatch value number at {}", i);
                return false;
            }
        }
    }
    // Register numbers should align across the registerLifetimes arrays.
    for (size_t i = 0; i < linearBlock->registerLifetimes.size(); ++i) {
        for (const auto& lt : linearBlock->registerLifetimes[i]) {
            if (lt.registerNumber != i) {
                SPDLOG_ERROR("Mismatch register number at {}", i);
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

#endif // HADRON_PIPELINE_VALIDATE

} // namespace hadron