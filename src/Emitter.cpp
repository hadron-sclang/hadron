#include "hadron/Emitter.hpp"

#include "hadron/BlockSerializer.hpp"
#include "hadron/HIR.hpp"
#include "hadron/JIT.hpp"

namespace hadron {

void Emitter::emit(LinearBlock* linearBlock, JIT* jit) {
    for (size_t line = 0; line < linearBlock->instructions.size(); ++line) {
        const hir::HIR* hir = linearBlock->instructions[line].get();
        switch(hir->opcode) {
        case kLoadArgument:
            break;
        }
    }
}

} // namespace hadron