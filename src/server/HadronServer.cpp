#include "server/HadronServer.hpp"

#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/Emitter.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Interpreter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/Parser.hpp"
#include "hadron/RegisterAllocator.hpp"
#include "hadron/Resolver.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/VirtualJIT.hpp"
#include "server/JSONTransport.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

namespace server {

HadronServer::HadronServer(std::unique_ptr<JSONTransport> jsonTransport):
        m_jsonTransport(std::move(jsonTransport)),
        m_state(kUninitialized),
        m_interpreter(std::make_unique<hadron::Interpreter>()) {
    m_jsonTransport->setServer(this);
}

int HadronServer::runLoop() {
    return m_jsonTransport->runLoop();
}

void HadronServer::initialize(std::optional<lsp::ID> id) {
    if (!m_interpreter->setup()) {
        m_jsonTransport->sendErrorResponse(id, JSONTransport::ErrorCode::kInternalError,
                "Failed to setup Hadron Interpreter.");
        return;
    }
    m_state = kRunning;
    m_jsonTransport->sendInitializeResult(id);
}

void HadronServer::semanticTokensFull(const std::string& filePath) {
    hadron::SourceFile sourceFile(filePath);
    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    if (!sourceFile.read(errorReporter)) {
        m_jsonTransport->sendErrorResponse(std::nullopt, JSONTransport::ErrorCode::kFileReadError,
                fmt::format("Failed to read file {} for lexing.", filePath));
        return;
    }

    auto code = sourceFile.codeView();
    hadron::Lexer lexer(code, errorReporter);
    if (!lexer.lex() || !errorReporter->ok()) {
        // TODO: error reporting
        return;
    }

    m_jsonTransport->sendSemanticTokens(lexer.tokens());
}

void HadronServer::hadronCompilationDiagnostics(lsp::ID id, const std::string& filePath) {
    hadron::SourceFile sourceFile(filePath);
    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    if (!sourceFile.read(errorReporter)) {
        m_jsonTransport->sendErrorResponse(std::nullopt, JSONTransport::ErrorCode::kFileReadError,
                fmt::format("Failed to read file {} for parsing.", filePath));
        return;
    }

    auto code = sourceFile.codeView();
    hadron::Lexer lexer(code, errorReporter);
    if (!lexer.lex() || !errorReporter->ok()) {
        // TODO: errorReporter starts reporting problems itself
        return;
    }

    hadron::Parser parser(&lexer, errorReporter);
    if (!parser.parse() || !errorReporter->ok()) {
        // TODO: errors
        return;
    }

    SPDLOG_TRACE("Compile Diagnostics Block Builder");
    hadron::BlockBuilder blockBuilder(&lexer, errorReporter);
    auto frame = blockBuilder.buildFrame(reinterpret_cast<const hadron::parse::BlockNode*>(parser.root()));

    SPDLOG_TRACE("Compile Diagnostics Block Serializer");
    hadron::BlockSerializer blockSerializer;
    auto linearBlock = blockSerializer.serialize(std::move(frame), hadron::LighteningJIT::physicalRegisterCount());

    SPDLOG_TRACE("Compile Diagnostics Lifetime Analyzer");
    hadron::LifetimeAnalyzer lifetimeAnalyzer;
    lifetimeAnalyzer.buildLifetimes(linearBlock.get());

    SPDLOG_TRACE("Compile Diagnostics Register Allocator");
    hadron::RegisterAllocator registerAllocator;
    registerAllocator.allocateRegisters(linearBlock.get());

    SPDLOG_TRACE("Compile Diagnostics Resolver");
    hadron::Resolver resolver;
    resolver.resolve(linearBlock.get());

//    SPDLOG_TRACE("Compile Diagnostics Emitter");
//    hadron::Emitter emitter;
    hadron::VirtualJIT virtualJIT;
//    emitter.emit(linearBlock.get(), &virtualJIT);

    // Rebuid frame to include in diagnostics.
    hadron::BlockBuilder blockRebuilder(&lexer, errorReporter);
    frame = blockRebuilder.buildFrame(reinterpret_cast<const hadron::parse::BlockNode*>(parser.root()));
    m_jsonTransport->sendCompilationDiagnostics(id, parser.root(), frame.get(), linearBlock.get(), &virtualJIT);
}



} // namespace server