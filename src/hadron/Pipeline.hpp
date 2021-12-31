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

#include <memory>
#include <string_view>

namespace hadron {

class ErrorReporter;

// Utility class to compile an input string into machine code. Includes optional code to validate the compiler state
// between each step in the pipeline, and virtual methods for additional validation work per-step.
class Pipeline {
public:
    Pipeline();
    explicit Pipeline(std::shared_ptr<ErrorReporter> errorReporter);
    ~Pipeline();

    bool compileBlock(std::string_view code);
    // bool compileMethod(const parse::MethodNode* method, ) ??

#if HADRON_PIPELINE_VALIDATE
    // With pipeline validation on these methods are called after internal validation of each step. Their default
    // implementions do nothing. They are intended primarily for use by the Pipeline unittests, allowing for additional
    // testing work on each pipeline step as needed.
    virtual bool afterBlockBuilder();
    virtual bool afterBlockSerializer();
    virtual bool afterLifetimeAnalyzer();
    virtual bool afterRegsiterAllocator();
    virtual bool afterResolver();
#endif // HADRON_PIPELINE_VALIDATE

protected:
    std::shared_ptr<ErrorReporter> m_errorReporter;
};

} // namespace hadron

#endif // SRC_HADRON_PIPELINE_HPP_