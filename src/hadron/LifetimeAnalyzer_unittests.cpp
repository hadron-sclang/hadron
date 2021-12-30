#include "hadron/LifetimeAnalyzer.hpp"

#include "hadron/Block.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Hash.hpp"
#include "hadron/HIR.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/Parser.hpp"

#include "doctest/doctest.h"

namespace {

static constexpr size_t kNumberOfTestRegisters = 16;

// There are some subtleties about block ranges, phis, and loops, which should be checked for correct behavior in
// individual test cases. The broad invariant this function checks is that all acesses of a value happen while it
// is live, and should also be in the usages set.
void validateLifetimes(const hadron::LinearBlock* linearBlock) {
    std::vector<size_t> usageCounts(linearBlock->valueLifetimes.size(), 0);
    for (size_t i = 0; i < linearBlock->instructions.size(); ++i) {
        const auto hir = linearBlock->instructions[i].get();
        if (hir->value.isValid()) {
            CHECK(linearBlock->valueLifetimes[hir->value.number][0].covers(i));
            ++usageCounts[hir->value.number];
        }
        for (const auto& value : hir->reads) {
            CHECK(linearBlock->valueLifetimes[value.number][0].covers(i));
            CHECK_EQ(linearBlock->valueLifetimes[value.number][0].usages.count(i), 1);
            ++usageCounts[value.number];
        }
    }
    for (size_t i = 0; i < linearBlock->valueLifetimes.size(); ++i) {
        CHECK_EQ(linearBlock->valueLifetimes[i][0].valueNumber, i);
        CHECK_EQ(linearBlock->valueLifetimes[i][0].usages.size(), usageCounts[i]);
    }
}

std::unique_ptr<hadron::LinearBlock> analyze(const char* code) {
    hadron::Parser parser(code);
    REQUIRE(parser.parse());
    REQUIRE(parser.root() != nullptr);
    REQUIRE(parser.root()->nodeType == hadron::parse::NodeType::kBlock);
    const auto block = reinterpret_cast<const hadron::parse::BlockNode*>(parser.root());
    hadron::BlockBuilder builder(parser.lexer(), parser.errorReporter());
    auto frame = builder.buildFrame(block);
    REQUIRE(frame);
    hadron::BlockSerializer serializer;
    auto linearBlock = serializer.serialize(std::move(frame), kNumberOfTestRegisters);
    REQUIRE(linearBlock);
    hadron::LifetimeAnalyzer analyzer;
    analyzer.buildLifetimes(linearBlock.get());
    validateLifetimes(linearBlock.get());
    return linearBlock;
}

} // namespace

namespace hadron {

TEST_CASE("LifetimeAnalyzer Simple Blocks") {
    SUBCASE("nil block") {
        analyze("nil");
    }
}

} // namespace hadron