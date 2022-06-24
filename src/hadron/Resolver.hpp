#ifndef SRC_COMPILER_INCLUDE_HADRON_RESOLVER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_RESOLVER_HPP_

#include "hadron/library/HadronLinearFrame.hpp"

namespace hadron {

struct ThreadContext;

// The Resolver takes a LinearFrame that has completed register allocation, and schedules the various register transfers
// required both to keep values consistent across register allocation changes as well as between different blocks across
// control flow. This class implements the RESOLVE algorithm described in [RA5], "Linear Scan Register Allocation on SSA
// Form." by C. Wimmer and M. Franz.
class Resolver {
public:
    Resolver() = default;
    ~Resolver() = default;

    void resolve(ThreadContext* context, library::LinearFrame linearFrame);

private:
    bool findAt(int32_t valueNumber, library::LinearFrame linearFrame, int32_t line, int32_t& location);
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_RESOLVER_HPP_