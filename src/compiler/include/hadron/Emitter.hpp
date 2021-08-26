#ifndef SRC_COMPILER_INCLUDE_HADRON_EMITTER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_EMITTER_HPP_

namespace hadron {

class JIT;
struct LinearBlock;

// The Emitter takes a completed LinearBlock and emits machine code using the provided JIT class.
class Emitter {
public:
    Emitter() = default;
    ~Emitter() = default;

    void emit(LinearBlock* linearBlock, JIT* jit);
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_EMITTER_HPP_