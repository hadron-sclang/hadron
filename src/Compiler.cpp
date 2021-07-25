#include "hadron/Compiler.hpp"

#include "FileSystem.hpp"
#include "hadron/CodeGenerator.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Function.hpp"
#include "hadron/JITMemoryArena.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/MachineCodeRenderer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SyntaxAnalyzer.hpp"
#include "hadron/VirtualJIT.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

Compiler::Compiler():
    m_jitMemoryArena(std::make_unique<JITMemoryArena>()),
    m_errorReporter(std::make_shared<ErrorReporter>()),
    m_quit(false) {}

Compiler::~Compiler() { stop(); }

bool Compiler::start(size_t numberOfThreads) {
    if (m_jitMemoryArena->createArena()) {
        SPDLOG_ERROR("Compiler failed to create JIT Memory Arena.");
        return false;
    }

    if (numberOfThreads == 0) {
        unsigned hardwareThreads = std::thread::hardware_concurrency();
        if (hardwareThreds > 4) {
            numberOfThreads = (hardwareThreads / 2) - 1;
        } else {
            numberOfThreads = 1;
        }
    }

    SPDLOG_INFO("Compiler starting {} threads.", numberOfThreads);
    for (auto i = 0; i < numberOfThreads; ++i) {
        m_compilerThreads.emplace_back(std::thread(&Compiler::compilerThreadMain, this, i));
    }

    return true;
}

void Compiler::stop() {
    if (!m_quit) {
        m_quit = true;
        m_jobQueueCondition.notify_all();
        for (auto& thread : m_compilerThreads) {
            thread.join();
        }
        m_compilerThreads.clear();

        SPDLOG_DEBUG("Compiler terminated with {} jobs left in queue.", m_jobQueue.size());
    }
}

void Compiler::compile(std::string_view code, std::function<void(std::unique_ptr<Function>)> func) {
    {
        std::lock_guard<std::mutex> lock(m_jobQueueMutex);
        m_jobQueue.emplace_back([this, code, result])() { asyncCompile(code, func); });
    }
    m_jobQueueCondition.notify_one();
}

void Compiler::compilerThreadMain(size_t threadNumber) {
    SPDLOG_DEBUG("Compiler thread {} entry.", threadNumber);

    LighteningJIT::markThreadForJITCompilation();

    while (!m_quit) {
        std::function<void()> workFunction;
        bool hasWork = false;

        {
            std::unique_lock<std::mutex> lock(m_jobQueueMutex);
            m_jobQueueCondition.wait(lock, [this] { return m_quit || m_jobQueue.size(); });
            if (m_quit) {
                break;
            }

            if (m_jobQueue.size()) {
                workFunction = m_jobQueue.front();
                m_jobQueue.pop_front();
                hasWork = true;
            }
        }

        while (!m_quit && hasWork) {
            workFunction();
            {
                std::lock_guard<std::mutex> lock(m_jobQueueMutex);
                if (m_jobQueue.size()) {
                    workFunction = m_jobQueue.front();
                    m_jobQueue.pop_front();
                } else {
                    hasWork = false;
                }
            }
        }
    }

    SPDLOG_DEBUG("Compiler thread {} normal exit.", threadNumber);
}

// TODO: RAII-style callback wrapper for func?
void Compiler::asyncCompile(std::string_view code, std::function<void(std::unique_ptr<Function>)> func) {
    SPDLOG_DEBUG("Compiling '{}'", std::string(code.data(), code.size()));

    auto errorReporter = std::make_shared<ErrorReporter>();
    Lexer lexer(code, errorReporter);
    if (!lexer.lex() || !errorReporter->ok()) {
        SPDLOG_DEBUG("Lexing failed, firing empty callback.");
        func(nullptr);
        return;
    }

    Parser parser(&lexer, errorReporter);
    if (!parser.parse() || !errorReporter->ok()) {
        SPDLOG_DEBUG("Parsing failed, firing empty callback.");
        func(nullptr);
        return;
    }

    SyntaxAnalyzer analyzer(&parser, errorReporter);
    if (!analyzer.buildAST() || !errorReporter->ok()) {
        SPDLOG_DEBUG("Analysis failed, firing empty callback.");
        func(nullptr);
        return;
    }

    //**  check that root AST is a block type.
    if (analyzer.ast()->astType != ast::ASTType::kBlock) {
        // This is an errorReporter error instead of SPDLOG because the error is a problem with user input.
        errorReporter->addError("Not a block!");
        func(nullptr);
        return;
    }

    const ast::BlockAST* blockAST = reinterpret_cast<const ast::BlockAST*>(analyzer.ast());
    CodeGenerator generator(blockAST, errorReporter);
    if (!generator.generate() || !errorReporter->ok()) {
        SPDLOG_DEBUG("Code Generation failed, firing empty callback.");
        func(nullptr);
        return;
    }

    // Estimate jit buffer size to be the byte size of the generated IR bytecode + some constant size for entry and
    // exit trampolines. If this is too large by far it's potentially wasteful, we can call xallocx() to downsize the
    // buffer, an extended jemalloc function that doesn't copy buffers when reallocating. It is unknown what kind of
    // pressure this will cause on the memory allocator but I assume the allocator performs better on allocations
    // when the size doesn't change after allocation. Note that a a buffer copy, like realloc() may perform, will break
    // the JIT code inside, as it is not relocateable. If the machine code ends up being larger then the allocated
    // buffer we double the projected size and regenerate the JIT, repeating until the generated code fits. Therefore
    // accurate estimates of generated bytecode size are useful in preventing excessive memory waste or re-rendering.
    size_t machineCodeSize = (generator.virtualJIT()->instructions().size() * sizeof(VirtualJIT::Inst)) + 4096;
    auto machineCode = m_jitMemoryArena->alloc(machineCodeSize);
    if (!machineCode) {
        SPDLOG_CRITICAL("Failed to allocate JIT memory!");
        func(nullptr);
        return;
    }

    LighteningJIT jit;
    jit.begin(machineCode.get(), machineCodeSize);

    // Build function object JIT C entry trampoline.
    auto function = std::make_unique<Function>(blockAST);

    MachineCodeRenderer renderer(generator.virtualJIT(), errorReporter);
    if (!renderer.render() || !errorReporter->ok()) {
        SPDLOG_DEBUG("Code Rendering failed, firing empty callback".);
        func(nullptr);
        return;
    }
}

} // namespace hadron
