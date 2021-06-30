#include "Block.hpp"

namespace hadron {

Block* Block::findContainingScope(uint64_t nameHash) {
    if (values.find(nameHash) != values.end()) {
        return this;
    }
    if (scopeParent) {
        return scopeParent->findContainingScope(nameHash);
    }
    return nullptr;
 }

} // namespace hadron