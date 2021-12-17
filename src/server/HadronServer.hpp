#ifndef SRC_SERVER_HADRON_SERVER_HPP_
#define SRC_SERVER_HADRON_SERVER_HPP_

#include "server/CompilationUnit.hpp"
#include "server/LSPTypes.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace hadron {
class ErrorReporter;
class Interpreter;
class Lexer;
namespace parse {
struct BlockNode;
} // namespace parse
} // namespace hadron

namespace server {

class JSONTransport;

class HadronServer {
public:
    HadronServer() = delete;
    HadronServer(std::unique_ptr<JSONTransport> jsonTransport);

    int runLoop();

    // LSP commands
    void initialize(std::optional<lsp::ID> id);
    void semanticTokensFull(const std::string& filePath);

    void hadronCompilationDiagnostics(lsp::ID id, const std::string& filePath);

    enum ServerState {
        kUninitialized,
        kRunning,
        kShutdownRequested,
    };
    ServerState state() const { return m_state; }

private:
    void addCompilationUnit(std::string name, const hadron::Lexer* lexer, const hadron::parse::BlockNode* blockNode,
            std::vector<CompilationUnit>& units);

    std::unique_ptr<JSONTransport> m_jsonTransport;
    ServerState m_state;
    std::shared_ptr<hadron::ErrorReporter> m_errorReporter;

    std::unique_ptr<hadron::Interpreter> m_interpreter;
};

} // namespace server

#endif // SRC_SERVER_HADRON_SERVER_HPP_