#include "server/HadronServer.hpp"

#include "hadron/Arch.hpp"
#include "hadron/ASTBuilder.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/ClassLibrary.hpp"
#include "hadron/Emitter.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LighteningJIT.hpp"
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
    if (!m_runtime->initInterpreter()) {
        m_jsonTransport->sendErrorResponse(id, JSONTransport::ErrorCode::kInternalError,
                "Failed to initialize Hadron runtime.");
        return;
    }

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

} // namespace server