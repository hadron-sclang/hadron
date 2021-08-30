#ifndef SRC_SERVER_JSON_TRANSPORT_HPP_
#define SRC_SERVER_JSON_TRANSPORT_HPP_

#include "server/LSPTypes.hpp"

#include <memory>
#include <optional>
#include <stdio.h>
#include <string>

namespace server {

class HadronServer;

// Credit is due for this class to the authors clangd, which inspired much of the design and implementation of this
// class.
class JSONTransport {
public:
    JSONTransport() = delete;
    // Non-owning input pointers. These are using the stdio input mechanisms because of some documentation in the clangd
    // source code that indicates compatibility and reliability issues on the input stream when using C++ stdlib stream
    // interfaces.
    JSONTransport(FILE* inputStream, FILE* outputStream);
    ~JSONTransport();
    void setServer(HadronServer* server);

    // The main run loop, returns an exit status code.
    int runLoop();

    // Communication back from the server.
    enum ErrorCode : int {
        kParseError = -32700,
        kInvalidRequest = -32600,
        kMethodNotFound = -32601,
        kInvalidParams = -32602,
        kInternalError = -32603,
        kServerNotInitialized = -32002,
        kUnknownErrorCode = -32001,
        kContentModified = -32801,
        kRequestCanceled = -32800
    };
    void sendErrorResponse(std::optional<lsp::ID> id, ErrorCode errorCode, std::string errorMessage);
    void sendInitializeResult(std::optional<lsp::ID> id);

private:
    // pIMPL pattern to keep JSON headers from leaking into rest of server namespace.
    class JSONTransportImpl;
    std::unique_ptr<JSONTransportImpl> m_impl;
};

} // namespace server

#endif // SRC_SERVER_JSON_TRANSPORT_HPP_