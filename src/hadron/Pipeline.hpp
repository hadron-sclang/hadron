#ifndef SRC_HADRON_PIPELINE_HPP_
#define SRC_HADRON_PIPELINE_HPP_

// By default we turn off pipeline validation in Release builds.
#ifndef HADRON_PIPELINE_VALIDATE
#ifdef NDEBUG
#define HADRON_PIPELINE_VALIDATE 0
#else
#define HADRON_PIPELINE_VALIDATE 1
#endif // NDEBUG
#endif // HADRON_PIPELINE_VALIDATE

#include "hadron/Block.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/lir/LIR.hpp"
#include "hadron/Slot.hpp"

#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace hadron {

namespace ast {
struct BlockAST;
}

struct Block;
class ErrorReporter;
struct Frame;
class Lexer;
struct LinearFrame;
struct Scope;
struct ThreadContext;

// Utility class to compile an input string into machine code. Includes optional code to validate the compiler state
// between each step in the pipeline, and virtual methods for additional validation work per-step.
class Pipeline {
public:
    Pipeline();
    explicit Pipeline(std::shared_ptr<ErrorReporter> errorReporter);
    ~Pipeline();

    // Parameters to override before compilation, or leave at defaults.
    size_t numberOfRegisters() const { return m_numberOfRegisters; }
    void setNumberOfRegisters(size_t n) { m_numberOfRegisters = n; }

    bool jitToVirtualMachine() const { return m_jitToVirtualMachine; }
    void setJitToVirtualMachine(bool useVM) { m_jitToVirtualMachine = useVM; }

    library::FunctionDef compileCode(ThreadContext* context, std::string_view code);
    library::FunctionDef compileBlock(ThreadContext* context, const ast::BlockAST* blockAST);

    library::Method compileMethod(ThreadContext* context, const library::Class classDef, const ast::BlockAST* blockAST);

#if HADRON_PIPELINE_VALIDATE
    // With pipeline validation on these methods are called after internal validation of each step. Their default
    // implementions do nothing. They are intended primarily for use by the Pipeline unittests, allowing for additional
    // testing work on each pipeline step as needed. Any method that returns false will stop the pipeline from moving
    // to the next step.
    virtual bool afterBlockBuilder(const Frame* frame, const ast::BlockAST* blockAST);
    virtual bool afterBlockSerializer(const LinearFrame* linearFrame);
    virtual bool afterLifetimeAnalyzer(const LinearFrame* linearFrame);
    virtual bool afterRegisterAllocator(const LinearFrame* linearFrame);
    virtual bool afterResolver(const LinearFrame* linearFrame);
    virtual bool afterEmitter(const LinearFrame* linearFrame, library::Int8Array bytecodeArray);
#endif // HADRON_PIPELINE_VALIDATE

protected:
    void setDefaults();
    bool buildBlock(ThreadContext* context, const ast::BlockAST* blockAST, library::FunctionDef functionDef);

#if HADRON_PIPELINE_VALIDATE
    // Checks for valid SSA form and that all members of Frame and contained Blocks are valid.
    bool validateFrame(ThreadContext* context, const Frame* frame, const ast::BlockAST* blockAST);
    bool validateSubScope(const Scope* scope, const Scope* parent, std::unordered_set<Block::ID>& blockIds);

    bool validateSerializedBlock(const LinearFrame* linearFrame, size_t numberOfBlocks);
    bool validateSsaLir(const lir::LIR* lir, std::unordered_set<lir::VReg>& values);

    bool validateLifetimes(const LinearFrame* linearFrame);

    bool validateAllocation(const LinearFrame* linearFrame);
    bool validateRegisterCoverage(const LinearFrame* linearFrame, size_t i, uint32_t vReg);

    bool validateResolution(const LinearFrame* linearFrame);

    bool validateEmission(const LinearFrame* linearFrame, library::Int8Array bytecodeArray);
#endif // HADRON_PIPELINE_VALIDATE

    std::shared_ptr<ErrorReporter> m_errorReporter;
    size_t m_numberOfRegisters;
    bool m_jitToVirtualMachine;
};

} // namespace hadron

#endif // SRC_HADRON_PIPELINE_HPP_