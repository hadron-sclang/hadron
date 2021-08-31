#include "server/JSONTransport.hpp"

#include "server/HadronServer.hpp"
#include "server/LSPMethods.hpp"

#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "spdlog/spdlog.h"

#include <array>
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
    void sendErrorResponse(std::optional<lsp::ID> id, ErrorCode errorCode, std::string errorMessage);
    void sendInitializeResult(std::optional<lsp::ID> id);

private:
    size_t readHeaders();
    void sendMessage(rapidjson::Document& document);
    bool handleMethod(const std::string& methodName, std::optional<lsp::ID> id, rapidjson::Document& document);
    bool handleResponse(lsp::ID id, rapidjson::Document& document);
    void encodeId(std::optional<lsp::ID> id, rapidjson::Document& document);

    void handleInitialize(std::optional<lsp::ID> id, const rapidjson::GenericObject<false, rapidjson::Value>& params);

    FILE* m_inputStream;
    FILE* m_outputStream;
    HadronServer* m_server;
};

JSONTransport::JSONTransportImpl::JSONTransportImpl(FILE* inputStream, FILE* outputStream):
        m_inputStream(inputStream), m_outputStream(outputStream), m_server(nullptr) {}

int JSONTransport::JSONTransportImpl::runLoop() {
    std::vector<char> json;
    SPDLOG_INFO("runLoop entry");

    while (!feof(m_inputStream)) {
        size_t contentLength = readHeaders();
        if (contentLength == 0) {
            continue;
        }

        // JSON objects longer than a megabyte fail to parse.
        if (contentLength > 1024 * 1024) {
            SPDLOG_CRITICAL("Rejecting Content-Length: {} > 1 MB.", contentLength);
            return -1;
        }

        json.resize(contentLength + 1);
        size_t bytesRead = 0;
        while (bytesRead != contentLength) {
            size_t read = fread(json.data() + bytesRead, 1, contentLength - bytesRead, m_inputStream);
            if (read == 0 || ferror(m_inputStream)) {
                SPDLOG_CRITICAL("Input read failure.");
                return -1;
            }
            bytesRead += read;
        }
        SPDLOG_TRACE("Read {} JSON bytes", contentLength);

        rapidjson::Document document;
        rapidjson::ParseResult parseResult = document.Parse(json.data());
        if (!parseResult) {
            sendErrorResponse(std::nullopt, ErrorCode::kParseError, "Failed to parse input JSON.");
            continue;
        }
        SPDLOG_TRACE("Parsed input JSON.");

        // Validate some basic properties of the received JSON-RPC object
        if (!document.IsObject()) {
            sendErrorResponse(std::nullopt, ErrorCode::kInvalidRequest, "Input JSON is not a JSON object.");
            continue;
        }
        if (!document.HasMember("jsonrpc")) {
            sendErrorResponse(std::nullopt, ErrorCode::kInvalidRequest, "Input JSON missing 'jsonrpc' key.");
            continue;
        }
        if (!document["jsonrpc"].IsString()) {
            sendErrorResponse(std::nullopt, ErrorCode::kInvalidRequest, "Input 'jsonrpc' key is not a string.");
            continue;
        }
        rapidjson::Value& jsonrpc = document["jsonrpc"];
        if (std::strcmp(jsonrpc.GetString(), "2.0") != 0) {
            sendErrorResponse(std::nullopt, ErrorCode::kInvalidRequest, fmt::format(
                    "Unsupported 'jsonrpc' value of '{}'.", jsonrpc.GetString()));
            continue;
        }
        SPDLOG_TRACE("Got valid JSONRPC.");

        std::optional<lsp::ID> id;
        if (document.HasMember("id")) {
            if (document["id"].IsString()) {
                id = std::string(document["id"].GetString());
            } else if (document["id"].IsNumber()) {
                id = document["id"].GetInt64();
            } else {
                sendErrorResponse(std::nullopt, ErrorCode::kInvalidRequest,
                        "Type for 'id' key must be string or number.");
                continue;
            }
        }
        std::optional<std::string> method;
        if (document.HasMember("method")) {
            if (document["method"].IsString()) {
                method = std::string(document["method"].GetString());
            } else {
                sendErrorResponse(id, ErrorCode::kInvalidRequest, "Type for 'method' key must be string.");
                continue;
            }
        }

        //           | no ID        | ID       |
        // ----------+--------------+----------+
        // no method | *invalid*    | response |
        // method    | notification | request  |
        if (!method) {
            if (!id) {
                sendErrorResponse(id, ErrorCode::kInvalidRequest, "Message missing both 'id' and 'method' keys.");
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
    SPDLOG_ERROR("Sending error code: {}, message: {}", errorCode, errorMessage);
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

void JSONTransport::JSONTransportImpl::sendInitializeResult(std::optional<lsp::ID> id) {
    rapidjson::Document document;
    document.SetObject();
    document.AddMember("jsonrpc", rapidjson::Value("2.0"), document.GetAllocator());
    encodeId(id, document);
    rapidjson::Value result;
    result.SetObject();
    rapidjson::Value capabilities;
    capabilities.SetObject();
    result.AddMember("capabilities", capabilities, document.GetAllocator());
    rapidjson::Value serverInfo;
    serverInfo.SetObject();
    serverInfo.AddMember("name", rapidjson::Value("hlangd"), document.GetAllocator());
    serverInfo.AddMember("version", rapidjson::Value("0.0.1"), document.GetAllocator());
    result.AddMember("serverInfo", serverInfo, document.GetAllocator());
    document.AddMember("result", result, document.GetAllocator());
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
            SPDLOG_TRACE("Parsed Content-Length header with {} bytes", contentLength);
        } else if (headerLine.size() == 2 && headerLine.front() == '\r') {
            // empty line with only \r\n indicates end of headers
            SPDLOG_TRACE("Parsed end of headers.");
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
        rapidjson::Document& document) {
    auto method = server::lsp::getMethodNamed(methodName.data(), methodName.size());

    // Spec calls to reject all messages sent to server before kInitialize
    if (m_server->state() == HadronServer::ServerState::kUninitialized && method != lsp::Method::kInitialize) {
        sendErrorResponse(id, ErrorCode::kServerNotInitialized, fmt::format("Rejecting method {}, server not "
            "initialized.", methodName));
        return false;
    }

    switch(method) {
    case server::lsp::Method::kNotFound:
        sendErrorResponse(id, ErrorCode::kMethodNotFound, fmt::format("Failed to match method '{}' to supported name.",
            methodName));
        return false;

    case server::lsp::Method::kInitialize: {
        if (!document.HasMember("params") || !document["params"].IsObject()) {
            sendErrorResponse(id, ErrorCode::kInvalidParams, "Absent or malformed params key in 'initialize' method.");
            return false;
        }
        handleInitialize(id, document["params"].GetObject());
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

void JSONTransport::JSONTransportImpl::handleInitialize(std::optional<lsp::ID> id,
        const rapidjson::GenericObject<false, rapidjson::Value>& /* params */) {
    m_server->initialize(id);
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

void JSONTransport::sendErrorResponse(std::optional<lsp::ID> id, ErrorCode errorCode, std::string errorMessage) {
    m_impl->sendErrorResponse(id, errorCode, errorMessage);
}

void JSONTransport::sendInitializeResult(std::optional<lsp::ID> id) {
    m_impl->sendInitializeResult(id);
}


} // namespace server