#ifndef SRC_HADRON_HIR_HPP_
#define SRC_HADRON_HIR_HPP_

#include "Literal.hpp"
#include "Value.hpp"

#include <string_view>
#include <variant>
#include <vector>

namespace hadron {

// Hadron IR (Intermediate Representation) is constructed from the parse tree and represents an intermediate step
// between syntax and low-level IR or machine code generation. Hadron HIR is suitable for insertion into Blocks for
// building of Control Flow Graphs, and can be processed into SSA form.

// The goals are to disambiguate the abstract symbol names provided by the syntax tree into actual variable references
// and method calls. This IR should be suitable, with some further processing, to convert into either LLVM IR or
// GNU Lightning IR as output. LLVM takes care of register packing, Lightning does not. HIR deals only with Values,
// typed and named entities.
enum Opcode {
    /* function calls */
    // Should resolve function calls to all first-class types (Slot is a first-class type)
    // Push then on stack in left-to-right order
    // For dispatch to another previously compiled sclang method the first argument is the
    // selector hash, next is the target, then followed by the arguments
    kPrepare,  // Set up stack frame for function call
    kPushArgI32,  // Add an Int32 argument
    kFinish, // Needs string argument with function name?
    kDispatch, // Could be a specialized opcode for intralangauge calls

    /* arithmetic */
    kAddI32, // add 2 i32 args, store result

    /* comparisons */
    kLessThanI32, // first op = second op < third op

    /* Value Assignment */
    kAssignI32, // assign value of variable or constant to argument

    /* Flow Control (these are terminal instructions in a Block) */
    kBranchIf // conditional branch, takes a single operand
};

struct HIR {
    Opcode opcode;
    using Operand = std::variant<std::monostate, ValueRef, Literal, std::string_view>;
    std::vector<Operand> operands;

    HIR(Opcode op): opcode(op) {}
    HIR(Opcode op, std::vector<Operand>&& ops): opcode(op), operands(ops) {}
};

} // namespace hadron

#endif // SRC_HADRON_HIR_HPP_