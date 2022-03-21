#ifndef SRC_HADRON_NAME_SAVER_HPP_
#define SRC_HADRON_NAME_SAVER_HPP_

#include "hadron/Block.hpp"
#include "hadron/hir/HIR.hpp"

#include "hadron/library/Symbol.hpp"

#include <memory>
#include <unordered_map>

namespace hadron {

class ErrorReporter;
struct Frame;
struct ThreadContext;

namespace hir {
struct AssignHIR;
} // namespace hir

// Analyzes an input CFG to determine which named values need to be saved to the heap, including instance and class
// variables, as well as any captured local variables. Removes redundant AssignHIR statements resulting from graph
// optimizations.

// This is a required step in compilation, between BlockBuilder and BlockSerializer.
class NameSaver {
public:
    NameSaver() = delete;
    NameSaver(ThreadContext* context, std::shared_ptr<ErrorReporter> errorReporter);
    ~NameSaver() = default;

    void scanFrame(Frame* frame);

private:
    void scanBlock(Block* block, std::unordered_set<Block::ID>& visitedBlocks);

    ThreadContext* m_threadContext;
    std::shared_ptr<ErrorReporter> m_errorReporter;

    enum NameType {
        kClass,    // class variable
        kExternal, // external value that may require capture if modified
        kInstance, // instance variable
        kLocal     // argument or local variable
    };
    struct NameState {
        NameType type;
        hir::ID initialValue;
        hir::ID value;
        int32_t index;
        hir::AssignHIR* assign;
    };
    std::unordered_map<library::Symbol, NameState> m_nameStates;
};

} // namespace hadron

#endif // SRC_HADRON_NAME_SAVER_HPP_