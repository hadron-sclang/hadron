#include "server/HadronServer.hpp"

#include "hadron/Arch.hpp"
#include "hadron/Block.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/Emitter.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/Parser.hpp"
#include "hadron/RegisterAllocator.hpp"
#include "hadron/Resolver.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/VirtualJIT.hpp"
#include "server/JSONTransport.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

namespace server {

HadronServer::HadronServer(std::unique_ptr<JSONTransport> jsonTransport):
        m_jsonTransport(std::move(jsonTransport)),
        m_state(kUninitialized),
        m_errorReporter(std::make_shared<hadron::ErrorReporter>()),
        m_runtime(std::make_unique<hadron::Runtime>(m_errorReporter)) {
    m_jsonTransport->setServer(this);
}

int HadronServer::runLoop() {
    return m_jsonTransport->runLoop();
}

void HadronServer::initialize(std::optional<lsp::ID> id) {
/*
    if (!m_runtime->initialize()) {
        m_jsonTransport->sendErrorResponse(id, JSONTransport::ErrorCode::kInternalError,
                "Failed to initialize Hadron runtime.");
        return;
    }
*/
    m_state = kRunning;
    m_jsonTransport->sendInitializeResult(id);
}

void HadronServer::semanticTokensFull(const std::string& filePath) {
    hadron::SourceFile sourceFile(filePath);
    if (!sourceFile.read(m_errorReporter)) {
        m_jsonTransport->sendErrorResponse(std::nullopt, JSONTransport::ErrorCode::kFileReadError,
                fmt::format("Failed to read file {} for lexing.", filePath));
        return;
    }

    auto code = sourceFile.codeView();
    hadron::Lexer lexer(code, m_errorReporter);
    if (!lexer.lex() || !m_errorReporter->ok()) {
        // TODO: error reporting
        return;
    }

    m_jsonTransport->sendSemanticTokens(lexer.tokens());
}

void HadronServer::hadronCompilationDiagnostics(lsp::ID id, const std::string& filePath) {
    hadron::SourceFile sourceFile(filePath);
    if (!sourceFile.read(m_errorReporter)) {
        m_jsonTransport->sendErrorResponse(std::nullopt, JSONTransport::ErrorCode::kFileReadError,
                fmt::format("Failed to read file {} for parsing.", filePath));
        return;
    }

    auto code = sourceFile.codeView();
    hadron::Lexer lexer(code, m_errorReporter);
    if (!lexer.lex() || !m_errorReporter->ok()) {
        // TODO: errorReporter starts reporting problems itself
        return;
    }

    // This is not a good way to determine "classNess" of a file. The best fix is likely going to
    // be to restructure the SC grammar to be able to mix class definitions and interpreted code
    // more freely. A better medium-term fix is likely going to be adjusting the protocol to
    // better clarify user intent - are they wanting to see a class file dump or a script dump?
    // But for now, as these APIs are still very plastic, we key off of file extension.
    bool isClassFile = filePath.size() > 3 && (filePath.substr(filePath.size() - 3, 3) == ".sc");

    hadron::Parser parser(&lexer, m_errorReporter);
    std::vector<CompilationUnit> units;

    // Determine if the input file was an interpreter script or a class file and parse accordingly.
    if (isClassFile) {
        if (!parser.parseClass() || !m_errorReporter->ok()) {
            // TODO: error handling
            return;
        }
        const hadron::parse::Node* node = parser.root();
        while (node) {
            std::string name;
            const hadron::parse::MethodNode* method = nullptr;
            if (node->nodeType == hadron::parse::NodeType::kClass) {
                auto classNode = reinterpret_cast<const hadron::parse::ClassNode*>(node);
                name = std::string(lexer.tokens()[classNode->tokenIndex].range);
                method = classNode->methods.get();
            } else if (node->nodeType == hadron::parse::NodeType::kClassExt) {
                auto classExtNode = reinterpret_cast<const hadron::parse::ClassExtNode*>(node);
                name = "+" + std::string(lexer.tokens()[classExtNode->tokenIndex].range);
                method = classExtNode->methods.get();
            }
            while (method) {
                std::string methodName = name + ":" + std::string(lexer.tokens()[method->tokenIndex].range);
                addCompilationUnit(methodName, &lexer, method->body.get(), units);
                method = reinterpret_cast<const hadron::parse::MethodNode*>(method->next.get());
            }

            node = node->next.get();
        }
    } else {
        if (!parser.parse() || !m_errorReporter->ok()) {
            // TODO: errors
            return;
        }
        assert(parser.root()->nodeType == hadron::parse::NodeType::kBlock);
        addCompilationUnit("INTERPRET", &lexer, reinterpret_cast<const hadron::parse::BlockNode*>(parser.root()),
                units);
    }
    m_jsonTransport->sendCompilationDiagnostics(id, units);
}

void HadronServer::addCompilationUnit(std::string name, const hadron::Lexer* lexer,
        const hadron::parse::BlockNode* blockNode, std::vector<CompilationUnit>& units) {
    // TODO: can this be refactored to use hadron::Pipeline?
    SPDLOG_TRACE("Compile Diagnostics Block Builder {}", name);
    hadron::BlockBuilder blockBuilder(lexer, m_errorReporter);
    auto frame = blockBuilder.buildFrame(blockNode);

    SPDLOG_TRACE("Compile Diagnostics Block Serializer {}", name);
    hadron::BlockSerializer blockSerializer;
    auto linearBlock = blockSerializer.serialize(std::move(frame));

    SPDLOG_TRACE("Compile Diagnostics Lifetime Analyzer {}", name);
    hadron::LifetimeAnalyzer lifetimeAnalyzer;

    lifetimeAnalyzer.buildLifetimes(linearBlock.get());

    SPDLOG_TRACE("Compile Diagnostics Register Allocator {}", name);
    hadron::RegisterAllocator registerAllocator(hadron::kNumberOfPhysicalRegisters);
    registerAllocator.allocateRegisters(linearBlock.get());

    SPDLOG_TRACE("Compile Diagnostics Resolver {}", name);
    hadron::Resolver resolver;
    resolver.resolve(linearBlock.get());

    SPDLOG_TRACE("Compile Diagnostics Emitter {}", name);
    hadron::Emitter emitter;
    hadron::VirtualJIT jit;
    size_t byteCodeSize = linearBlock->instructions.size() * 16;
    auto byteCode = std::make_unique<uint8_t[]>(byteCodeSize);
    jit.begin(byteCode.get(), byteCodeSize);
    emitter.emit(linearBlock.get(), &jit);
    size_t finalSize = 0;
    jit.end(&finalSize);
    assert(finalSize < byteCodeSize);

    // Rebuid frame to include in diagnostics.
    hadron::BlockBuilder blockRebuilder(lexer, m_errorReporter);
    frame = blockRebuilder.buildFrame(blockNode);

    units.emplace_back(CompilationUnit{name, blockNode, std::move(frame), std::move(linearBlock),
            std::move(byteCode), byteCodeSize});
}

} // namespace server