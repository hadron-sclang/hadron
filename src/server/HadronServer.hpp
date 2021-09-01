#ifndef SRC_SERVER_HADRON_SERVER_HPP_
#define SRC_SERVER_HADRON_SERVER_HPP_

#include "server/LSPTypes.hpp"

#include <memory>
#include <optional>
#include <string>

namespace hadron {
class Interpreter;
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

    // Hadron commands
    void hadronParseTree(lsp::ID id, const std::string& filePath);

    enum ServerState {
        kUninitialized,
        kRunning,
        kShutdownRequested,
    };
    ServerState state() const { return m_state; }

private:
    std::unique_ptr<JSONTransport> m_jsonTransport;
    ServerState m_state;

    std::unique_ptr<hadron::Interpreter> m_interpreter;
};

} // namespace server

#endif // SRC_SERVER_HADRON_SERVER_HPP_