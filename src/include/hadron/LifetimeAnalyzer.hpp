#ifndef SRC_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_
#define SRC_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_

namespace hadron {

struct LinearBlock;

//   Lexer: convert input string into tokens
//   Parser: build parse tree from input tokens
//   SSABuilder: convert parse tree to CFG/HIR format
//   BlockSerializer: flatten to linear block
//   LifetimeAnalyzer: lifetime analysis (still needs successor info from CFG)
//   RegisterAllocator: register allocation
//   MachineCodeGenerator: SSA form desconstruction/HIR -> machine code translation

class LifetimeAnalyzer {
public:
    LifetimeAnalyzer() = default;
    ~LifetimeAnalyzer() = default;

    void buildLifetimes(LinearBlock* linearBlock);
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_LIFETIME_ANALYZER_HPP_