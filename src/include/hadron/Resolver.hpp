#ifndef SRC_INCLUDE_HADRON_RESOLVER_HPP_
#define SRC_INCLUDE_HADRON_RESOLVER_HPP_

namespace hadron {

class JIT;
class LinearBlock;

// The Resolver takes a LinearBlock that has completed register allocation and generates ouptut machine code to the
// provided JIT object. This class implements the RESOLVE algorithm described in [RA5], "Linear Scan Register
// Allocation on SSA Form." by C. Wimmer and M. Franz.
class Resolver {
public:
    Resolver() = default;
    ~Resolver() = default;

    void resolve(LinearBlock* linearBlock, JIT* jit);
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_RESOLVER_HPP_