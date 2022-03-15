#include "hadron/Validator.hpp"

#include "hadron/Frame.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/lir/LabelLIR.hpp"
#include "hadron/Scope.hpp"
#include "hadron/ThreadContext.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

// static
bool Validator::validateFrame(const Frame* frame) {
    int32_t argumentOrderArraySize = frame->argumentOrder.size();
    int32_t argumentDefaultsArraySize = frame->argumentDefaults.size();
    if (argumentOrderArraySize != argumentDefaultsArraySize) {
        SPDLOG_ERROR("Frame has mismatched argument order and defaults array sizes of {} and {} respectively",
            argumentOrderArraySize, argumentDefaultsArraySize);
        return false;
    }

    std::unordered_set<Block::ID> blockIds;
    std::unordered_set<hir::ID> valueIds;
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

// static
bool Validator::validateSubScope(const Scope* scope, const Scope* parent, std::unordered_set<Block::ID>& blockIds,
        std::unordered_set<hir::ID>& valueIds) {
    if (scope->parent != parent) {
        SPDLOG_ERROR("Scope parent mismatch");
        return false;
    }
    for (const auto& block : scope->blocks) {
        // Block must have a reference back to the correct owning scope.
        if (block->scope != scope) {
            SPDLOG_ERROR("Block frame mismatch");
            return false;
        }
        // Block ids must be unique.
        if (blockIds.find(block->id) != blockIds.end()) {
            SPDLOG_ERROR("Non-unique block number {}", block->id);
            return false;
        }

        // All blocks must be sealed.
        if (!block->isSealed) {
            SPDLOG_ERROR("Block {} is not sealed.", block->id);
            return false;
        }

        for (const auto& phi : block->phis) {
            if (valueIds.count(phi->id)) {
                SPDLOG_ERROR("Duplicate ID {} found in phi in block {}", phi->id, block->id);
                return false;
            }
            if (scope->frame->values[phi->id] != phi.get()) {
                SPDLOG_ERROR("Mismatch in phi between value id and pointer for ID {}", phi->id);
                return false;
            }
            valueIds.emplace(phi->id);
        }

        for (const auto& hir : block->statements) {
            if (hir->id != hir::kInvalidID) {
                if (valueIds.count(hir->id)) {
                    SPDLOG_ERROR("Duplicate ID {} found for hir in block {}", hir->id, block->id);
                    return false;
                }
                if (scope->frame->values[hir->id] != hir.get()) {
                    SPDLOG_ERROR("Mismatch between value id and pointer for ID {}", hir->id);
                    return false;
                }
                valueIds.emplace(hir->id);
            }
        }

        blockIds.emplace(block->id);
    }
    for (const auto& subScope : scope->subScopes) {
        if (!validateSubScope(subScope.get(), scope, blockIds, valueIds)) { return false; }
    }

    return true;
}

// static
bool Validator::validateLinearFrame(const LinearFrame* linearFrame, size_t numberOfBlocks) {
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

// static
bool Validator::validateSsaLir(const lir::LIR* lir, std::unordered_set<lir::VReg>& values) {
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

// static
bool Validator::validateLifetimes(const LinearFrame* linearFrame) {
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


// static
bool Validator::validateAllocation(const hadron::LinearFrame* linearFrame) {
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

// static
bool Validator::validateResolution(const LinearFrame*) {
    // TODO: Could go through and look at boundaries for each block, validating that the expectations of where values
    // are have been met from each predecessor block.
    return true;
}

// static
bool Validator::validateEmission(const LinearFrame*, library::Int8Array) {
    return true;
}

// static
bool Validator::validateRegisterCoverage(const LinearFrame* linearFrame, size_t i, uint32_t vReg) {
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

} // namespace hadron