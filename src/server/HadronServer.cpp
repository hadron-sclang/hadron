#include "server/HadronServer.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Interpreter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SourceFile.hpp"
#include "server/JSONTransport.hpp"

#include "fmt/format.h"

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

void HadronServer::semanticTokensFull(lsp::ID id, const std::string& filePath) {
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
        // TODO: error reporting
        return;
    }
}

void HadronServer::hadronParseTree(lsp::ID id, const std::string& filePath) {
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

    m_jsonTransport->sendParseTree(id, parser.root());
}

} // namespace server