#ifndef SRC_SERVER_HADRON_SERVER_HPP_
#define SRC_SERVER_HADRON_SERVER_HPP_

#include <memory>

namespace server {

class JSONTransport;

class HadronServer {
public:
    HadronServer() = delete;
    HadronServer(std::unique_ptr<JSONTransport> jsonTransport);

    int runLoop();

    enum ServerState {
        kUninitialized,
        kRunning,
        kShutdownRequested,
    };
    ServerState state() const { return m_state; }

private:
    std::unique_ptr<JSONTransport> m_jsonTransport;
    ServerState m_state;
};

} // namespace server

#endif // SRC_SERVER_HADRON_SERVER_HPP_