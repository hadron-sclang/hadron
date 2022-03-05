#ifndef SRC_HADRON_SCOPE_HPP_
#define SRC_HADRON_SCOPE_HPP_

#include <list>
#include <memory>
#include <unordered_set>

#include "hadron/Hash.hpp"
#include "hadron/library/Symbol.hpp"

namespace hadron {

struct Frame;
struct Block;

// A Scope is a area of the code where variable declarations are valid. All Blocks of code execute within one or more
// nested Scopes. Scopes must have a singular entry point, meaning the first Block within a Scope must exist and must
// have at most a single predecessor.
struct Scope {
    Scope() = delete;
    // Make an entry Scope to a frame, so it has no parent Scope.
    explicit Scope(Frame* owningFrame): frame(owningFrame), parent(nullptr) {}
    // Make a subscope for the existing parent scope.
    explicit Scope(Scope* parentScope): frame(parentScope->frame), parent(parentScope) {}
    ~Scope() = default;

    // Set of locally defined variable names.
    #error you're not doing anything with this.
    std::unordered_set<library::Symbol> variableNames;

    Frame* frame;
    Scope* parent;
    std::list<std::unique_ptr<Block>> blocks;
    std::list<std::unique_ptr<Scope>> subScopes;
};

} // namespace hadron

#endif // SRC_HADRON_SCOPE_HPP_