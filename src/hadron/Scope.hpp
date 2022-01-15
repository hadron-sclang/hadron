#ifndef SRC_HADRON_SCOPE_HPP_
#define SRC_HADRON_SCOPE_HPP_

#include <list>
#include <memory>
#include <unordered_set>

#include "hadron/Hash.hpp"

namespace hadron {

struct Frame;
struct Block;

// A Scope is a area of the code where variable declarations are valid. All Blocks of code execute within one or more
// nested Scopes.
struct Scope {
    Scope() = delete;
    Scope(Frame* owningFrame, Scope* parentScope): frame(owningFrame), parent(parentScope) {}
    ~Scope() = default;

    // Set of locally defined variable names
    std::unordered_set<Hash> variableNames;

    Frame* frame;
    Scope* parent;
    std::list<std::unique_ptr<Block>> blocks;
    std::list<std::unique_ptr<Scope>> subScopes;
};

} // namespace hadron

#endif // SRC_HADRON_SCOPE_HPP_