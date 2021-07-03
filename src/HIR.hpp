#ifndef SRC_HIR_HPP_
#define SRC_HIR_HPP_

#include <array>
#include <variant>

namespace hadron {

// Hadron Intermediate Represention, or HIR, matches a subset of the GNU lightning instruction set.
struct HIR {
    enum Opcode {
        addr,  // add integer registers O1 = O2 + O3
        addi,  // add integer immediate O1 = O2 + O3

    };

    using Register = int;
    using IntImmediate = int;
    using FloatImmediate = float;
    using Label = int;
    using Operand = std::variant<std::monostate, Register, IntImmediate, FloatImmediate, Label>;
    using Operands = std::array<Operand, 3>;
    Opcode opcode;
    Operands operands;
};

} // namespace hadron

#endif // SRC_HIR_HPP_