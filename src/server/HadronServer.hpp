#ifndef SRC_SERVER_HADRON_SERVER_HPP_
#define SRC_SERVER_HADRON_SERVER_HPP_

#include "hadron/library/Kernel.hpp"
#include "server/CompilationUnit.hpp"
#include "server/LSPTypes.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace hadron {
class ErrorReporter;
class Lexer;
class Parser;
class Runtime;
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

    // Responses from the server for Hadron messages
    enum DiagnosticsStoppingPoint : int {
        kAST = 1,
        kFrame = 2,
        kLowering = 3,
        kLifetimeAnalysis = 4,
        kRegisterAllocation = 5,
        kResolution = 6,
        kMachineCodeEmission = 7
    };
    void hadronCompilationDiagnostics(lsp::ID id, const std::string& filePath, DiagnosticsStoppingPoint stopAfter);

    enum ServerState {
        kUninitialized,
        kRunning,
        kShutdownRequested,
    };
    ServerState state() const { return m_state; }

private:
    void addCompilationUnit(hadron::library::Method methodDef, std::shared_ptr<hadron::Lexer> lexer,
            std::shared_ptr<hadron::Parser> parser, const hadron::parse::BlockNode* blockNode,
            std::vector<CompilationUnit>& units, DiagnosticsStoppingPoint stopAfter);

    std::unique_ptr<JSONTransport> m_jsonTransport;
    ServerState m_state;
    std::shared_ptr<hadron::ErrorReporter> m_errorReporter;

    std::unique_ptr<hadron::Runtime> m_runtime;
};

} // namespace server

#endif // SRC_SERVER_HADRON_SERVER_HPP_