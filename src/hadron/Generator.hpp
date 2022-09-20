#ifndef SRC_HADRON_GENERATOR_HPP_
#define SRC_HADRON_GENERATOR_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/HadronCFG.hpp"
#include "hadron/library/Integer.hpp"

#if defined(__clang__) || defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif

#if defined(__i386__)
#elif defined(__x86_64__)
#    include "asmjit/x86.h"
#elif defined(__arm__)
#elif defined(__aarch64__)
#    include "asmjit/a64.h"
#endif

#if defined(__clang__) || defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include <vector>

// Thinking about generating three functions:
//   a) Innermost function takes exactly the number of arguments (and later types) expected
//   b) Wrapper function to correct args: f(context, int numArgs, ... args) <- args has inOrder followed by key/value
//   c) Selector-specific dispatch function that handles routing

namespace hadron {

class Generator {
public:
    Generator();
    ~Generator() = default;

    void serialize(ThreadContext* context, const library::CFGFrame frame);

private:
    // Performs a recursive postorder traversal of the blocks and saves the output in |blockOrder|.
    void orderBlocks(ThreadContext* context, library::CFGBlock block, std::vector<library::CFGBlock>& blocks,
                     library::TypedArray<library::BlockId> blockOrder);
    void buildFunction(const library::CFGFrame frame, asmjit::FuncSignature signature,
                       std::vector<library::CFGBlock>& blocks, library::TypedArray<library::BlockId> blockOrder);

    asmjit::JitRuntime m_jitRuntime; // needs to last for the lifetime of the program
    asmjit::CodeHolder m_codeHolder; // needs to last until compilation is complete
};

} // namespace hadron

#endif // SRC_HADRON_GENERATOR_HPP_
