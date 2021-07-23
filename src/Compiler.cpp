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

    CodeGenerator generator(reinterpret_cast<ast::BlockAST*>(analyzer.ast()), errorReporter);
    if (!generator.generate() || !errorReporter->ok()) {
        SPDLOG_DEBUG("Code Generation failed, firing empty callback.");
        func(nullptr);
        return;
    }

    MachineCodeRenderer renderer(generator.virtualJIT(), errorReporter);
    LighteningJIT jit;
    
}

} // namespace hadron
