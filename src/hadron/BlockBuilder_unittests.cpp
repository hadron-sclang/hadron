#include "hadron/BlockBuilder.hpp"

#include "hadron/Block.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Hash.hpp"
#include "hadron/HIR.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"

#include "doctest/doctest.h"

#include <unordered_map>
#include <unordered_set>

namespace {

void validateHIR(const hadron::hir::HIR* hir, std::unordered_map<uint32_t, uint32_t>& values,
        const hadron::Block* block) {
    // Unique values should only be written to once.
    CHECK_EQ(values.find(hir->value.number), values.end());
    for (const auto& read : hir->reads) {
        // Read values should all exist already and with the same type ornamentation.
        auto value = values.find(read.number);
        REQUIRE_NE(value, values.end());
        CHECK_EQ(read.typeFlags, value->second);
        // Read values should also exist in the local value map.
        CHECK_NE(block->localValues.find(read), block->localValues.end());
    }

    values.emplace(std::make_pair(hir->value.number, hir->value.typeFlags));
}

void validateSubFrame(const hadron::Frame* frame, const hadron::Frame* parent,
        std::unordered_map<uint32_t, uint32_t>& values, std::unordered_set<int>& blockNumbers) {
    CHECK_EQ(frame->parent, parent);
    for (const auto& block : frame->blocks) {
        // Block must have a reference back to the correct owning frame.
        CHECK_EQ(block->frame, frame);
        // Block numbers must be unique.
        CHECK_EQ(blockNumbers.find(block->number), blockNumbers.end());
        blockNumbers.emplace(block->number);

        for (const auto& phi : block->phis) {
            validateHIR(phi.get(), values, block.get());
        }
        for (const auto& hir : block->statements) {
            validateHIR(hir.get(), values, block.get());
        }
    }
    for (const auto& subFrame : frame->subFrames) {
        validateSubFrame(subFrame.get(), frame, values, blockNumbers);
    }
}

// Checks for valid SSA form and that all members of Frame and contained Blocks are valid.
void validateFrame(const hadron::Parser* parser, const hadron::parse::BlockNode* blockNode,
        const hadron::Frame* frame) {
    // Validate all arguments are present and in correct order and the first one is *this*.
    REQUIRE_GE(frame->argumentOrder.size(), 1);
    CHECK_EQ(frame->argumentOrder[0], hadron::kThisHash);
    size_t numberOfArguments = 1;
    if (blockNode->arguments && blockNode->arguments->varList) {
        const hadron::parse::VarDefNode* varDef = blockNode->arguments->varList->definitions.get();
        while (varDef) {
            auto nameHash = hadron::hash(parser->lexer()->tokens()[varDef->tokenIndex].range);
            REQUIRE_GE(frame->argumentOrder.size(), numberOfArguments);
            CHECK_EQ(frame->argumentOrder[numberOfArguments], nameHash);
            ++numberOfArguments;
            varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(varDef->next.get());
        }
    }
    CHECK_EQ(frame->argumentOrder.size(), numberOfArguments);

    std::unordered_map<uint32_t, uint32_t> values;
    std::unordered_set<int> blockNumbers;
    validateSubFrame(frame, nullptr, values, blockNumbers);

    CHECK_EQ(frame->numberOfValues, values.size());
    CHECK_EQ(frame->numberOfBlocks, static_cast<int>(blockNumbers.size()));
    // There should be at least one Block.
    CHECK_GE(blockNumbers.size(), 1);
}

}

namespace hadron {

TEST_CASE("BlockBuilder Simple Blocks") {
    SUBCASE("nil block") {
        Parser parser("nil");
        REQUIRE(parser.parse());
        REQUIRE(parser.root() != nullptr);
        REQUIRE(parser.root()->nodeType == parse::NodeType::kBlock);
        const auto block = reinterpret_cast<const parse::BlockNode*>(parser.root());

        BlockBuilder builder(parser.lexer(), parser.errorReporter());
        auto frame = builder.buildFrame(block);
        validateFrame(&parser, block, frame.get());
    }
}

} // namespace hadron