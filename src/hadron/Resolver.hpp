#ifndef SRC_COMPILER_INCLUDE_HADRON_RESOLVER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_RESOLVER_HPP_

#include <cstddef>

namespace hadron {

struct LinearFrame;

namespace hir {
struct MoveOperand;
}

// The Resolver takes a LinearFrame that has completed register allocation, and schedules the various register transfers
// required both to keep values consistent across register allocation changes as well as between different blocks across
// control flow. This class implements the RESOLVE algorithm described in [RA5], "Linear Scan Register Allocation on SSA
// Form." by C. Wimmer and M. Franz.
class Resolver {
public:
    Resolver() = default;
    ~Resolver() = default;

    void resolve(LinearFrame* linearFrame);

private:
    bool findAt(size_t valueNumber, LinearFrame* linearFrame, size_t line, int& location);
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_RESOLVER_HPP_