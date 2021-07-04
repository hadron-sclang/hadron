#ifndef SRC_LIGHTNING_JIT_HPP_
#define SRC_LIGHTNING_JIT_HPP_

#include <memory>

namespace hadron {

class Slot;
namespace ast {
struct AST;
struct BlockAST;
} // namespace ast

class JITBlock {
public:
    virtual Slot value() = 0;
};

class LightningJIT {
public:
    LightningJIT();
    ~LightningJIT();

    // Call once at start of all JIT sessions.
    static void initJITGlobals();
    static void finishJITGlobals();

    std::unique_ptr<JITBlock> jitBlock(const ast::BlockAST* block);
};

} // namespace hadron

#endif