#include "hadron/Frame.hpp"

#include "hadron/Block.hpp"
#include "hadron/hir/PhiHIR.hpp"
#include "hadron/Scope.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

Frame::Frame(hir::BlockLiteralHIR* outerBlock, library::Method meth, library::SymbolArray argOrder,
        library::Array argDefaults):
        outerBlockHIR(outerBlock),
        method(meth),
        argumentOrder(argOrder),
        argumentDefaults(argDefaults),
        hasVarArgs(false),
        rootScope(std::make_unique<Scope>(this)),
        numberOfBlocks(0) {}

void Frame::replaceValues(std::unordered_map<hir::HIR*, hir::HIR*>& replacements) {
    std::unordered_set<hir::HIR*> toRemove;

    while (replacements.size()) {
        auto iter = replacements.begin();
        auto orig = iter->first;
        assert(orig->id != hir::kInvalidID);

        auto repl = iter->second;
        assert(repl->id != hir::kInvalidID);

        replacements.erase(iter);

        SPDLOG_INFO("replacing {} with {}", orig->id, repl->id);

        // Update all consumers of this value with the replacement value.
        for (auto consumer : orig->consumers) {
            if (consumer == orig) { continue; }

            consumer->replaceInput(orig->id, repl->id);

            // Phis may themselves be rendered trivial by this replacement, if they are we append them to the list
            // of replacements.
            if (consumer->opcode == hir::Opcode::kPhi && toRemove.count(consumer) == 0) {
                auto phi = reinterpret_cast<hir::PhiHIR*>(consumer);
                auto trivialValue = phi->getTrivialValue();
                if (trivialValue != hir::kInvalidID) {
                    replacements[consumer] = values[trivialValue];
                }
            }

            // Add this new consumer to the replacement object.
            if (consumer != repl) { repl->consumers.emplace(consumer); }
        }

        // Update consumer pointers in any values orig HIR consumes.
        for (auto read : orig->reads) {
            hir::HIR* provider = values[read];
            assert(provider);
            if (provider != orig) { provider->consumers.erase(orig); }
            if (provider != repl) { provider->consumers.emplace(repl); }
        }

        toRemove.emplace(orig);
    }

    for (auto orig : toRemove) {
        // Now delete the original from its block.
        values[orig->id] = nullptr;

        if (orig->opcode == hir::Opcode::kPhi) {
            for (auto iter = orig->owningBlock->phis().begin(); iter != orig->owningBlock->phis().end(); ++iter) {
                if (orig->id == (*iter)->id) {
                    orig->owningBlock->phis().erase(iter);
                    break;
                }
            }
        } else {
            for (auto iter = orig->owningBlock->statements().begin(); iter != orig->owningBlock->statements().end();
                    ++iter) {
                if (orig->id == (*iter)->id) {
                    orig->owningBlock->statements().erase(iter);
                    break;
                }
            }
        }
    }
}


} // namespace hadron