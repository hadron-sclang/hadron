#ifndef SRC_HADRON_GENERATOR_AARCH64_HPP_
#define SRC_HADRON_GENERATOR_AARCH64_HPP_

#if !defined(__aarch64__)
#error Requires ARM64.
#endif

#include "hadron/library/Array.hpp"
#include "hadron/library/HadronCFG.hpp"
#include "hadron/library/Integer.hpp"

#if defined(__clang__) || defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include "asmjit/a64.h"

#if defined(__clang__) || defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

namespace hadron {

struct ThreadContext;

class Generator {
public:
    Generator();
    ~Generator() = default;

    void serialize(ThreadContext* context, const library::CFGFrame frame);

private:
    // Performs a recursive postorder traversal of the blocks and saves the output in |blockOrder|.
    void orderBlocks(ThreadContext* context, library::CFGBlock block, std::vector<library::CFGBlock>& blocks,
                     library::TypedArray<library::BlockId> blockOrder);

    asmjit::JitRuntime m_jitRuntime; // needs to last for the lifetime of the program - move to runtime
    asmjit::CodeHolder m_codeHolder; // needs to last until compilation is complete
};

} // namespace hadron

#endif // SRC_HADRON_GENERATOR_AARCH64_HPP_