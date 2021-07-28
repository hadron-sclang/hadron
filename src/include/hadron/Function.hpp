#ifndef SRC_HADRON_INCLUDE_FUNCTION_HPP_
#define SRC_HADRON_INCLUDE_FUNCTION_HPP_

#include "hadron/Hash.hpp"
#include "hadron/JITMemoryArena.hpp"
#include "hadron/LSBHashTable.hpp"
#include "hadron/Slot.hpp"

#include <memory>

namespace hadron {

class LighteningJIT;
struct ThreadContext;

namespace ast {
struct BlockAST;
}

// Represents a unit of executable SuperCollider code.
struct Function {
    Function() = delete;
    Function(const ast::BlockAST* block);
    ~Function() = default;

    int numberOfArgs;
    // Argument names in order.
    std::unique_ptr<Hash[]> argumentNames;
    std::unique_ptr<Slot[]> defaultValues;

    struct NameIndex {
        Hash hash;
        std::unique_ptr<NameIndex> next;
        int index;
    };
    struct NameTable : public LSBHashTable<NameIndex> {
        NameTable() : LSBHashTable() {}
        virtual ~NameTable() = default;
    };
    NameTable nameIndices;

    const uint8_t* machineCode;
    JITMemoryArena::MCodePtr machineCodeOwned;
};

}

#endif // SRC_HADRON_INCLUDE_FUNCTION_HPP_