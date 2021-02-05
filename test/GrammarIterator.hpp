#ifndef TEST_GRAMMAR_ITERATOR_HPP_
#define TEST_GRAMMAR_ITERATOR_HPP_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hadron {

class GrammarIterator {
public:
    bool buildGrammarTree();

    void logGrammarTree();
    void expand();
    size_t countExpansions();

private:
    struct GrammarRule;
    struct GrammarPattern {
        bool isRecursive = false;
        std::vector<std::string> termNames;
        std::vector<GrammarRule*> terms;

        size_t countExpansions(std::unordered_set<std::string>& visited) const;
    };

    struct GrammarRule {
        GrammarRule(std::string ruleName): name(ruleName) { }
        GrammarRule(std::string ruleName, std::string form): name(ruleName), trivialForm(form) { }

        void expand(std::unordered_map<std::string, size_t>& patternCounts, std::vector<GrammarRule*> ruleStack);
        size_t countExpansions(std::unordered_set<std::string>& visited) const;

        std::string name;
        std::string trivialForm;
        std::vector<std::unique_ptr<GrammarPattern>> patterns;
    };

    std::unordered_map<std::string, std::unique_ptr<GrammarRule>> m_ruleMap;
};

} // namespace hadron

// TODO: rename to GrammarIterator or some such.
// Alright so we're building a crazy tree from the recursive grammar. The grammar consists of named *rules*. Each rule
// has one or more *patterns*. Patterns are a sequence with other named rules or specific character sequences with '',
// the special <e> pattern which means that the rule is optional, or all upcase rules which indicate other character
// sequences. Patterns can be self-referential meaning they can include their containing rule. This makes generation of
// those difficult. It might be possible to test recursive parts of the grammar with simpler parts of it.
// idea: mark patterns as self-referential and add them to the end of the list of patterns, while adding the non-self
// referential ones to the end of the list. We also keep a string in each rule of the "simplest possible form" of that
// rule, determined by shortest string.

// The goal is to test at least one instance of every PATTERN, so we're looking for "trivial forms", meaning forms with
// subrules that were not recursive and so are likely to be shorter/simpler. Trivial forms are *only* to be used in
// recursive expansion, otherwise we miss out on testing on some of the other forms.

// So for example we want to test parsing of the "classes" rule:
// classes: <e> | classes classdef
// classdef: classname superclass '{' classvardecls methods '}'
//         | classname '[' optname ']' superclass '{' classvardecls methods '}'
//
// classname: CLASSNAME
// superclass: <e> | ':' classname
// classvardecls: <e> | classvardecls classvardecl
// etc... etc..

// We want to look at each pattern in the base rule, (there are 2). Then for each *term* in the individual patterns we
// do a recursive expansion on that term. That means another DFS rooted on that term, and all of the expansions
// associated with that. So in *exhaustive* testing there's going to be a lot of redundant testing of deeply nested
// patterns as they are expanded again and again. So the temptation has been to try and "prune" some of this redundant
// testing by thinking of "minimal" or "trivial" expansions that can be cached and then re-used in other expansions.
// There's also the problem of recursive grammars, some which are multiply recursive, such as:

// classes: <e> | classes classdef
// expr: expr '.' name '=' expr

// Trivial forms can be either empty strings or they can be as simple as possible, like for expr a single integer like
// 1 is a valid expr so we save it as that.

#endif // TEST_GRAMMAR_ITERATOR_HPP_
