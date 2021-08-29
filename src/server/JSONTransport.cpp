#include "server/JSONTransport.hpp"

#include "server/HadronServer.hpp"
#include "server/LSPMethods.hpp"

#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "spdlog/spdlog.h"

#include <array>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace server {

class JSONTransport::JSONTransportImpl {
public:
    JSONTransportImpl() = delete;
    JSONTransportImpl(FILE* inputStream, FILE* outputStream);
    ~JSONTransportImpl() = default;

    void setServer(HadronServer* server) { m_server = server; }
    int runLoop();

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

private:
    size_t readHeaders();
    void sendMessage(rapidjson::Document& document);
    bool handleMethod(const std::string& methodName, std::optional<lsp::ID> id, rapidjson::Document& document);
    bool handleResponse(lsp::ID id, rapidjson::Document& document);
    void encodeId(std::optional<lsp::ID> id, rapidjson::Document& document);

    FILE* m_inputStream;
    FILE* m_outputStream;
    HadronServer* m_server;
};

JSONTransport::JSONTransportImpl::JSONTransportImpl(FILE* inputStream, FILE* outputStream):
        m_inputStream(inputStream), m_outputStream(outputStream), m_server(nullptr) {}

int JSONTransport::JSONTransportImpl::runLoop() {
    std::vector<char> json;

    while (!feof(m_inputStream)) {
        size_t contentLength = readHeaders();
        if (contentLength == 0) {
            continue;
        }

        // JSON objects longer than a megabyte fail to parse.
        if (contentLength > 1024 * 1024) {
            SPDLOG_ERROR("Rejecting Content-Length: {} > 1 MB.", contentLength);
            return -1;
        }

        json.resize(contentLength + 1);
        size_t bytesRead = fread(json.data(), 1, contentLength, m_inputStream);
        if (bytesRead == 0 || ferror(m_inputStream)) {
            SPDLOG_ERROR("Input read failure.");
            return -1;
        }

        rapidjson::Document document;
        rapidjson::ParseResult parseResult = document.Parse(json.data());
        if (!parseResult) {
            SPDLOG_WARN("Failed to parse input JSON.");

            continue;
        }

        // Validate some basic properties of the received JSON-RPC object
        if (!document.IsObject() ||
            !document.HasMember("jsonrpc") ||
            !document["jsonrpc"].IsString() ||
            std::strcmp(document["jsonprc"].GetString(), "2.0") != 0) {
            SPDLOG_WARN("Input JSON not object or missing 'jsonrpc' key.");
            continue;
        }

        std::optional<lsp::ID> id;
        if (document.HasMember("id")) {
            if (document["id"].IsString()) {
                id = std::string(document["id"].GetString());
            } else if (document["id"].IsNumber()) {
                id = document["id"].GetInt64();
            } else {
                SPDLOG_WARN("Input JSON has invalid type for 'id' key.");
                continue;
            }
        }
        std::optional<std::string> method;
        if (document.HasMember("method")) {
            if (document["method"].IsString()) {
                method = std::string(document["method"].GetString());
            } else {
                SPDLOG_WARN("Invalid tgpe for 'method' key.");
                continue;
            }
        }

        //           | no ID        | ID       |
        // ----------+--------------+----------+
        // no method | *invalid*    | response |
        // method    | notification | request  |
        if (!method) {
            if (!id) {
                SPDLOG_WARN("Received message with neither 'method' or 'id' keys.");
                continue;
            }

            handleResponse(id.value(), document);
        } else {
            handleMethod(method.value(), id, document);
        }
    }

    SPDLOG_INFO("Normal exit from processing loop.");
    return 0;
}

void JSONTransport::JSONTransportImpl::sendErrorResponse(std::optional<lsp::ID> id, ErrorCode errorCode,
        std::string errorMessage) {
    rapidjson::Document document;
    document.SetObject();
    document.AddMember("jsonrpc", rapidjson::Value("2.0"), document.GetAllocator());
    encodeId(id, document);
    rapidjson::Value responseError;
    responseError.SetObject();
    responseError.AddMember("code", rapidjson::Value(static_cast<int>(errorCode)), document.GetAllocator());
    rapidjson::Value messageString;
    messageString.SetString(errorMessage.data(), document.GetAllocator());
    responseError.AddMember("message", messageString, document.GetAllocator());
    document.AddMember("error", responseError, document.GetAllocator());
    sendMessage(document);
}

// Parses the HTTP-style headers used in JSON-RPC. Ignores all but 'Content-Length:', which it returns the value of.
size_t JSONTransport::JSONTransportImpl::readHeaders() {
    std::array<char, 256> headerBuf;
    size_t contentLength = 0;

    while (!feof(m_inputStream)) {
        if (int fileError = ferror(m_inputStream)) {
            SPDLOG_ERROR("File error on input stream while reading headers.");
            return 0;
        }

        fgets(headerBuf.data(), 256, m_inputStream);
        std::string headerLine(headerBuf.data());

        if (headerLine.size() == 0) {
            continue;
        }

        // Header lines longer than 256 characters are parse errors for now.
        if (headerLine.size() == 256 && headerLine.back() != '\n') {
            SPDLOG_ERROR("Received a header line longer than 256 characters.");
            return 0;
        }

        static const std::string lengthHeader("Content-Length:");
        if (headerLine.substr(0, lengthHeader.size()) == lengthHeader) {
            contentLength = strtoll(headerLine.data() + lengthHeader.size(), nullptr, 10);
        } else if (headerLine.size() == 2 && headerLine.front() == '\r') {
            // empty line with only \r\n indicates end of headers
            return contentLength;
        }
        // some other header here, don't bother parsing
    }

    SPDLOG_INFO("Encountered EOF during header parsing.");
    return 0;
}


// Serialize and send a JSON-RPC message to the client.
void JSONTransport::JSONTransportImpl::sendMessage(rapidjson::Document& document) {
    // Serialize the document to memory to compute its length, needed for the JSON-RPC header.
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    std::string output = fmt::format("Content-Length: {}\r\n\r\n{}", buffer.GetSize(), buffer.GetString());
    fwrite(output.data(), 1, output.size(), m_outputStream);
}


bool JSONTransport::JSONTransportImpl::handleMethod(const std::string& methodName, std::optional<lsp::ID> id,
        rapidjson::Document& /* document */) {
    auto method = server::lsp::getMethodNamed(methodName.data(), methodName.size());

    // Spec calls to reject all messages sent to server before kInitialize
    if (m_server->state() == HadronServer::ServerState::kUninitialized && method != lsp::Method::kInitialize) {
        SPDLOG_WARN("Rejecting method {}, server not initialized.", methodName);
        sendErrorResponse(id, ErrorCode::kServerNotInitialized, "server not initialized");
    }

    switch(method) {
    case server::lsp::Method::kNotFound:
        SPDLOG_ERROR("Failed to match method '{}' to supported name.", methodName);
        return false;

    case server::lsp::Method::kInitialize: {
    } break;

    case server::lsp::Method::kInitialized:
        break;
    case server::lsp::Method::kShutdown:
        break;
    case server::lsp::Method::kExit:
        break;
    case server::lsp::Method::kLogTrace:
        break;
    case server::lsp::Method::kSetTrace:
        break;
    }

    return true;
}

bool JSONTransport::JSONTransportImpl::handleResponse(server::lsp::ID /* id */, rapidjson::Document& /* document */) {
    return true;
}

void JSONTransport::JSONTransportImpl::encodeId(std::optional<lsp::ID> id, rapidjson::Document& document) {
    if (id) {
        if (std::holds_alternative<int64_t>(*id)) {
            document.AddMember("id", rapidjson::Value(std::get<int64_t>(*id)), document.GetAllocator());
        } else {
            assert(std::holds_alternative<std::string>(*id));
            rapidjson::Value idString;
            idString.SetString(std::get<std::string>(*id).data(), document.GetAllocator());
            document.AddMember("id", idString, document.GetAllocator());
        }
    } else {
        // Encode id with a value of null.
        document.AddMember("id", rapidjson::Value(), document.GetAllocator());
    }
}

//////////////////
// JSONTransport
JSONTransport::JSONTransport(FILE* inputStream, FILE* outputStream):
    m_impl(std::make_unique<JSONTransportImpl>(inputStream, outputStream)) {}

// Default destructor in header complains about incomplete Impl type so declare it here and explicity delete the Impl.
JSONTransport::~JSONTransport() { m_impl.reset(); }

void JSONTransport::setServer(HadronServer* server) {
    m_impl->setServer(server);
}

int JSONTransport::runLoop() {
    return m_impl->runLoop();
}

} // namespace server