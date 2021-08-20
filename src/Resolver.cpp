#include "hadron/Resolver.hpp"

#include "hadron/JIT.hpp"
#include "hadron/SSABuilder.hpp"

/*
Pseudocode taken from [RA5] in the Bibliography, "Linear Scan Register Allocation on SSA Form." by C. Wimmer and M.
Franz.

RESOLVE
for each control flow edge from predecessor to successor do
    for each interval it live at begin of successor do
        if it starts at begin of successor then
            phi = phi function defining it
            opd = phi.inputOf(predecessor)
            if opd is a constant then
                moveFrom = opd
            else
                moveFrom = location of intervals[opd] at end of predecessor
        else
            moveFrom = location of it at end of predecessor
        moveTo = location of it at begin of successor
        if moveFrom ≠ moveTo then
            mapping.add(moveFrom, moveTo)

    mapping.orderAndInsertMoves()
*/

namespace hadron {

void Resolver::resolve(LinearBlock* /* linearBlock */, JIT* /* jit */) {
// TODO
}

} // namespace hadron