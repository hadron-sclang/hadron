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

namespace parse {
struct BlockNode;
} // namespace parse

struct Block;
class ErrorReporter;
class Heap;
struct Frame;
class Lexer;
struct LinearBlock;

// Utility class to compile an input string into machine code. Includes optional code to validate the compiler state
// between each step in the pipeline, and virtual methods for additional validation work per-step.
class Pipeline {
public:
    Pipeline();
    explicit Pipeline(std::shared_ptr<Heap> heap, std::shared_ptr<ErrorReporter> errorReporter);
    ~Pipeline();

    // Parameters to override before compilation, or leave at defaults.
    size_t numberOfRegisters() const { return m_numberOfRegisters; }
    void setNumberOfRegisters(size_t n) { m_numberOfRegisters = n; }

    // For interpreter code only, returns a LinearBlock structure ready for JIT emission, or nullptr on error.
    Slot compileBlock(std::string_view code);
    // bool compileMethod(const parse::MethodNode* method, ) ??

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
#endif // HADRON_PIPELINE_VALIDATE


protected:
    void setDefaults();
    std::unique_ptr<LinearBlock> buildBlock(const parse::BlockNode* blockNode, const Lexer* lexer);

#if HADRON_PIPELINE_VALIDATE
    // Checks for valid SSA form and that all members of Frame and contained Blocks are valid.
    bool validateFrame(const Frame* frame, const parse::BlockNode* blockNode, const Lexer* lexer);
    bool validateSubFrame(const Frame* frame, const Frame* parent, std::unordered_map<uint32_t, uint32_t>& values,
            std::unordered_set<int>& blockNumbers);
    bool validateFrameHIR(const hir::HIR* hir, std::unordered_map<uint32_t, uint32_t>& values, const Block* block);

    bool validateSerializedBlock(const LinearBlock* linearBlock, size_t numberOfBlocks, size_t numberOfValues);

    bool validateLifetimes(const LinearBlock* linearBlock);

    bool validateAllocation(const LinearBlock* linearBlock);
    bool validateRegisterCoverage(const LinearBlock* linearBlock, size_t i, uint32_t vReg);

    bool validateResolution(const LinearBlock* linearBlock);
#endif // HADRON_PIPELINE_VALIDATE

    std::shared_ptr<Heap> m_heap;
    std::shared_ptr<ErrorReporter> m_errorReporter;
    size_t m_numberOfRegisters;
};

} // namespace hadron

#endif // SRC_HADRON_PIPELINE_HPP_