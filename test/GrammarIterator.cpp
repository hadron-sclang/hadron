#include "GrammarIterator.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

size_t GrammarIterator::GrammarPattern::countExpansions(std::unordered_set<std::string>& visited) const {
    size_t count = 1;
    for (const auto term : terms) {
        if (visited.count(term->name) == 0) {
            count *= term->countExpansions(visited);
        }
    }
    return count;
}

void GrammarIterator::GrammarRule::expand(std::unordered_map<std::string, size_t>& /* patternCounts */,
        std::vector<GrammarRule*> /* ruleStack */) {
    // The idea is for each pattern in order, we start a recursive expansion on each term in the pattern. We have a map
    // that keeps the counts for the local expansion but this is created on the stack on each recursive subcall. We
    // terminate each subcall when we have a completed expansion (in the form of a string) for each term in the pattern.
    // This means keeping track of all iterations for each subexpansion and then counting through them each in order.
    // Exhaustive expansion would mean the outer product, so iterating through each subexpansion exhaustively requires
    // iterating through each term *completely*, for every other term in the pattern.

    // The stack is important to understand which GrammarRule to call to continue the expansion.
}

size_t GrammarIterator::GrammarRule::countExpansions(std::unordered_set<std::string>& visited) const {
    if (!patterns.size()) {
        return 1;
    }

    visited.insert(name);

    size_t count = 0;
    for (const auto& pattern : patterns) {
        count += pattern->countExpansions(visited);
    }

    visited.erase(name);
    return count;
}

void GrammarIterator::logGrammarTree() {
    for (const auto& rulePair : m_ruleMap) {
        GrammarRule* rule = rulePair.second.get();
        std::string p;
        for (const auto& pattern : rule->patterns) {
            for (const auto& r : pattern->termNames) {
                p = p + " " + r;
            }
            p = p + " | ";
        }
        spdlog::info("{}: {}", rule->name, p);
    }
}

void GrammarIterator::expand() {
    std::unordered_map<std::string, size_t> counts;
    std::vector<GrammarRule*> stack;
    GrammarRule* root = m_ruleMap.find("slotliteral")->second.get();
    root->expand(counts, stack);
}

size_t GrammarIterator::countExpansions() {
    GrammarRule* root = m_ruleMap.find("root")->second.get();
    std::unordered_set<std::string> visited;
    return root->countExpansions(visited);
}

}
