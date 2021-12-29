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

    REQUIRE_EQ(linearBlock->registerLifetimes.size(), kNumberOfTestRegisters);
    // Registers should be reserved for all Dispatch calls, as well as past the end of the block.

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

// validate ordering - maybe on different control structures, hand validation
// check blockRanges, valueLifetimes, registerLifetimes, spillLifetimes
// numberOfSpillSlots should be 1 always at this step

TEST_CASE("BlockSerializer Simple Blocks") {
    SUBCASE("nil block") {
        serialize("nil");
    }
}

} // namespace hadron