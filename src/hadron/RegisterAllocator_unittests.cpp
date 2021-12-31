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

// Check that at instruction |i| there is exactly one register allocated to |vReg|.
void validateRegisterCoverage(const hadron::LinearBlock* linearBlock, size_t i, uint32_t vReg) {
    int valueCovered = 0;
    size_t reg = 0;
    for (const auto& lt : linearBlock->valueLifetimes[vReg]) {
        if (lt.covers(i)) {
            CHECK_EQ(lt.usages.count(i), 1);
            ++valueCovered;
            reg = lt.registerNumber;
        }
    }
    CHECK_EQ(valueCovered, 1);

    int regCovered = 0;
    for (const auto& lt : linearBlock->registerLifetimes[reg]) {
        if (lt.covers(i)) {
            CHECK_EQ(lt.usages.count(i), 1);
            ++regCovered;
        }
    }
    CHECK_EQ(regCovered, 1);
}

void validateAllocation(const hadron::LinearBlock* linearBlock) {
    // Value numbers should align across the valueLifetimes arrays.
    for (size_t i = 0; i < linearBlock->valueLifetimes.size(); ++i) {
        for (const auto& lt : linearBlock->valueLifetimes[i]) {
            CHECK_EQ(lt.valueNumber, i);
        }
    }
    // Register numbers should align across the registerLifetimes arrays.
    for (size_t i = 0; i < linearBlock->registerLifetimes.size(); ++i) {
        for (const auto& lt : linearBlock->registerLifetimes[i]) {
            CHECK_EQ(lt.registerNumber, i);
        }
    }

    // Every usage of every virtual register should have a single physical register assigned.
    for (size_t i = 0; i < linearBlock->instructions.size(); ++i) {
        const auto* hir = linearBlock->instructions[i].get();
        if (hir->value.isValid()) {
            validateRegisterCoverage(linearBlock, i, hir->value.number);
        }
        for (const auto& value : hir->reads) {
            validateRegisterCoverage(linearBlock, i, value.number);
        }
    }
}

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
    validateAllocation(linearBlock.get());
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