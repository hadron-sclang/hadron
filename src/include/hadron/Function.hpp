#ifndef SRC_HADRON_INCLUDE_FUNCTION_HPP_
#define SRC_HADRON_INCLUDE_FUNCTION_HPP_

#include "hadron/Hash.hpp"
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

    bool buildEntryTrampoline(LighteningJIT* jit);

    int numberOfArgs;
    // Argument names in order.
    std::unique_ptr<Hash[]> argumentNames;
    std::unique_ptr<Slot[]> defaultValues;

    struct NameIndex {
        Hash hash;
        std::unique_ptr<NameIndex> next;
        int index;
    }
    struct NameTable : public LSBHashTable<NameIndex> {
        NameTable() : LSBHashTable() {}
        virtual ~NameTable() = default;
    }
    NameTable nameIndices;

    typedef void (*ExecJIT)();
    ExecJIT hadronEntry;

    // C++ wrapper to pack arguments into Hadron ABI, call into hadronEntry, catch the return, unpack the return
    // value, and return it.
    Slot value(ThreadContext* context, int numOrderedArgs, Slot* orderedArgs, int numKeywordArgs, Slot* keywordArgs);

private:
    ExecJIT cWrapper;
};

}

#endif // SRC_HADRON_INCLUDE_FUNCTION_HPP_