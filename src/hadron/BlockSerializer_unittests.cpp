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

std::unique_ptr<hadron::LinearBlock> serialize(const char* code) {
    hadron::Parser parser(code);
    REQUIRE(parser.parse());
    REQUIRE(parser.root() != nullptr);
    REQUIRE(parser.root()->nodeType == hadron::parse::NodeType::kBlock);
    const auto block = reinterpret_cast<const hadron::parse::BlockNode*>(parser.root());

    hadron::BlockBuilder builder(parser.lexer(), parser.errorReporter());
    auto frame = builder.buildFrame(block);
    hadron::BlockSerializer serializer;
    return serializer.serialize(std::move(frame), 16);
}

} // namespace

namespace hadron {

// validate ordering - maybe on different control structures, hand validation
// check blockRanges, valueLifetimes, registerLifetimes, spillLifetimes
// numberOfSpillSlots should be 1 always at this step

TEST_CASE("BlockSerializer Simple Blocks") {
    SUBCASE("nil block") {
        auto linearBlock = serialize("nil");
        REQUIRE(linearBlock);
    }
}

} // namespace hadron