#ifndef SRC_COMPILER_INCLUDE_HADRON_EMITTER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_EMITTER_HPP_

#include "hadron/library/HadronLinearFrame.hpp"

namespace hadron {

class JIT;
struct ThreadContext;

// The Emitter takes a completed LinearFrame and emits machine code using the provided JIT class.
class Emitter {
public:
    Emitter() = default;
    ~Emitter() = default;

    void emit(ThreadContext* context, library::LinearFrame linearFrame, JIT* jit);
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_EMITTER_HPP_