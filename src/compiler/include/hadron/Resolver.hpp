#ifndef SRC_COMPILER_INCLUDE_HADRON_RESOLVER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_RESOLVER_HPP_

#include <cstddef>

namespace hadron {

struct LinearBlock;

namespace hir {
struct MoveOperand;
}

// The Resolver takes a LinearBlock that has completed register allocation, and schedules the various register transfers
// required both to keep values consistent across register allocation changes as well as between different blocks across
// control flow. This class implements the RESOLVE algorithm described in [RA5], "Linear Scan Register Allocation on SSA
// Form." by C. Wimmer and M. Franz.
class Resolver {
public:
    Resolver() = default;
    ~Resolver() = default;

    void resolve(LinearBlock* linearBlock);

private:
    bool findAt(size_t valueNumber, LinearBlock* linearBlock, size_t line, int& location);
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_RESOLVER_HPP_