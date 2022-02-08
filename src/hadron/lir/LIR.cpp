#include "hadron/lir/LIR.hpp"

#include "hadron/MoveScheduler.hpp"

namespace hadron {
namespace lir {

void LIR::emit(JIT* jit) const {
    if (moves.size()) {
        MoveScheduler moveScheduler;
        moveScheduler.scheduleMoves(moves, jit);
    }
}

} // namespace lir
} // namespace hadron