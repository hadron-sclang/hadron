#include "hadron/BlockSerializer.hpp"

#include "hadron/Block.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Hash.hpp"
#include "hadron/HIR.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"

#include "doctest/doctest.h"

#include <memory>

namespace {

static constexpr size_t kNumberOfTestRegisters = 16;

// This validation is feeling very much like a "change detector" for the BlockSerializer class. However, the input
// requirements for the rest of the pipeline are specific so this serves as documentation and enforcement of those
// requiements. What the BlockSerializer does is somewhat "mechanical," too. It is likely there's a smarter way to test
// this code, so if the maintenence cost of this testing code becomes an undue burden please refactor. I expect the
// surfaces between BlockBuilder, BlockSerializer, and LifetimeAnalyzer to remain relatively stable, modulo algorithm
// changes, so the hope is that the serializer changes relatively infrequently and therefore the maintenence cost is low
// compared to the increased confidence that the inputs to the rest of the compiler pipeline are valid.
void validateBlock(const hadron::LinearBlock* linearBlock, size_t numberOfBlocks, size_t numberOfValues) {
    REQUIRE_EQ(linearBlock->blockOrder.size(), numberOfBlocks);
    REQUIRE_EQ(linearBlock->blockRanges.size(), numberOfBlocks);

    // The block order should see the ranges increasing with no gaps and covering all the instructions.
    size_t blockStart = 0;
    for (auto blockNumber : linearBlock->blockOrder) {
        REQUIRE_GT(linearBlock->blockRanges.size(), blockNumber);
        auto range = linearBlock->blockRanges[blockNumber];
        CHECK_EQ(range.first, blockStart);
        // Every block needs to begin with a label.
        REQUIRE_GT(linearBlock->instructions.size(), blockStart);
        CHECK_EQ(linearBlock->instructions[blockStart]->opcode, hadron::hir::Opcode::kLabel);
        // Next block should start at the end of this block.
        blockStart = range.second;
    }
    CHECK_EQ(linearBlock->instructions.size(), blockStart);

    // Value lifetimes should be the size of numberOfValues with one empty lifetime in each vector.
    REQUIRE_EQ(linearBlock->valueLifetimes.size(), numberOfValues);
    for (size_t i = 0; i < numberOfValues; ++i) {
        CHECK_EQ(linearBlock->valueLifetimes[i].size(), 1);
        CHECK(linearBlock->valueLifetimes[i][0].isEmpty());
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
    REQUIRE_EQ(linearBlock->registerLifetimes.size(), kNumberOfTestRegisters);
    for (size_t reg = 0; reg < kNumberOfTestRegisters; ++reg) {
        REQUIRE_EQ(linearBlock->registerLifetimes[reg].size(), 1);
        CHECK_EQ(linearBlock->registerLifetimes[reg][0].registerNumber, reg);
        REQUIRE_EQ(linearBlock->registerLifetimes[reg][0].ranges.size(), dispatchHIRIndices.size() + 1);
        auto rangeIter = linearBlock->registerLifetimes[reg][0].ranges.begin();
        for (auto index : dispatchHIRIndices) {
            CHECK_EQ(rangeIter->from, index);
            CHECK_EQ(rangeIter->to, index + 1);
            ++rangeIter;
        }
        CHECK_EQ(rangeIter->from, linearBlock->instructions.size());
        CHECK_GT(rangeIter->to, linearBlock->instructions.size());
    }

    // Spill lifetimes are empty until we finish register allocation.
    CHECK_EQ(linearBlock->spillLifetimes.size(), 0);

    // The spill slot counter should remain at the default until register allocation.
    CHECK_EQ(linearBlock->numberOfSpillSlots, 1);
}

std::unique_ptr<hadron::LinearBlock> serialize(const char* code) {
    hadron::Parser parser(code);
    REQUIRE(parser.parse());
    REQUIRE(parser.root() != nullptr);
    REQUIRE(parser.root()->nodeType == hadron::parse::NodeType::kBlock);
    const auto block = reinterpret_cast<const hadron::parse::BlockNode*>(parser.root());

    hadron::BlockBuilder builder(parser.lexer(), parser.errorReporter());
    auto frame = builder.buildFrame(block);
    REQUIRE(frame);
    size_t numberOfBlocks = static_cast<size_t>(frame->numberOfBlocks);
    size_t numberOfValues = frame->numberOfValues;
    hadron::BlockSerializer serializer;
    auto linearBlock = serializer.serialize(std::move(frame), kNumberOfTestRegisters);
    REQUIRE(linearBlock);
    validateBlock(linearBlock.get(), numberOfBlocks, numberOfValues);
    return linearBlock;
}

} // namespace

namespace hadron {

TEST_CASE("BlockSerializer Simple Blocks") {
    SUBCASE("nil block") {
        serialize("nil");
    }
}

} // namespace hadron