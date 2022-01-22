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

#include "hadron/Slot.hpp"

#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace hadron {

namespace hir {
struct HIR;
} // namespace hir

namespace library {
struct FunctionDef;
struct Int8Array;
struct Method;
}

namespace parse {
struct BlockNode;
} // namespace parse

struct Block;
class ErrorReporter;
struct Frame;
class Lexer;
struct LinearBlock;
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

    // For interpreter code only, returns an Int8Array with JIT bytecode, or nil on error.
    library::FunctionDef* compileCode(ThreadContext* context, std::string_view code);
    library::FunctionDef* compileBlock(ThreadContext* context, parse::BlockNode* blockNode, const Lexer* lexer);

    library::Method* compileMethod(ThreadContext* context, parse::MethodNode* methodNode, const Lexer* lexer,
            Slot classDef);

#if HADRON_PIPELINE_VALIDATE
    // With pipeline validation on these methods are called after internal validation of each step. Their default
    // implementions do nothing. They are intended primarily for use by the Pipeline unittests, allowing for additional
    // testing work on each pipeline step as needed. Any method that returns false will stop the pipeline from moving
    // to the next step.
    virtual bool afterBlockBuilder(const Frame* frame, const parse::BlockNode* blockNode, const Lexer* lexer);
    virtual bool afterBlockSerializer(const LinearBlock* linearBlock);
    virtual bool afterLifetimeAnalyzer(const LinearBlock* linearBlock);
    virtual bool afterRegisterAllocator(const LinearBlock* linearBlock);
    virtual bool afterResolver(const LinearBlock* linearBlock);
    virtual bool afterEmitter(const LinearBlock* linearBlock, Slot bytecodeArray);
#endif // HADRON_PIPELINE_VALIDATE

protected:
    void setDefaults();
    library::Int8Array* buildBlock(ThreadContext* context, const parse::BlockNode* blockNode, const Lexer* lexer);

#if HADRON_PIPELINE_VALIDATE
    // Checks for valid SSA form and that all members of Frame and contained Blocks are valid.
    bool validateFrame(ThreadContext* context, const Frame* frame, const parse::BlockNode* blockNode,
            const Lexer* lexer);
    bool validateSubScope(const Scope* scope, const Scope* parent, std::unordered_set<int>& blockNumbers);

    bool validateSerializedBlock(const LinearBlock* linearBlock, size_t numberOfBlocks, size_t numberOfValues);
    bool validateSsaHir(const hir::HIR* hir, std::unordered_set<uint32_t>& values);

    bool validateLifetimes(const LinearBlock* linearBlock);

    bool validateAllocation(const LinearBlock* linearBlock);
    bool validateRegisterCoverage(const LinearBlock* linearBlock, size_t i, uint32_t vReg);

    bool validateResolution(const LinearBlock* linearBlock);

    bool validateEmission(const LinearBlock* linearBlock, Slot bytecodeArray);
#endif // HADRON_PIPELINE_VALIDATE

    std::shared_ptr<ErrorReporter> m_errorReporter;
    size_t m_numberOfRegisters;
    bool m_jitToVirtualMachine;
};

} // namespace hadron

#endif // SRC_HADRON_PIPELINE_HPP_