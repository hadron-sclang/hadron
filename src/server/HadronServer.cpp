#include "server/HadronServer.hpp"

#include "hadron/Interpreter.hpp"
#include "server/JSONTransport.hpp"

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

} // namespace server