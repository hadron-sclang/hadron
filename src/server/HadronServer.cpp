#include "server/HadronServer.hpp"

#include "server/JSONTransport.hpp"

namespace server {

HadronServer::HadronServer(std::unique_ptr<JSONTransport> jsonTransport):
    m_jsonTransport(std::move(jsonTransport)) {}

int HadronServer::runLoop() {
    return m_jsonTransport->runLoop();
}

} // namespace server