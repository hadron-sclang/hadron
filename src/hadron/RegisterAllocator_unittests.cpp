#include "hadron/RegisterAllocator.hpp"

#include "hadron/Block.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Hash.hpp"
#include "hadron/HIR.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/Parser.hpp"

#include "doctest/doctest.h"

namespace {

static constexpr size_t kNumberOfTestRegisters = 16;

std::unique_ptr<hadron::LinearBlock> allocateRegisters(const char* code) {
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
    hadron::RegisterAllocator allocator;
    allocator.allocateRegisters(linearBlock.get());
    return linearBlock;
}

} // namespace

namespace hadron {

TEST_CASE("RegisterAllocator Simple Blocks") {
    SUBCASE("nil block") {
        allocateRegisters("nil");
    }
}

} // namespace hadron