#include "server/JSONTransport.hpp"

#include "server/LSPMethods.hpp"

#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace {

// Parses the HTTP-style headers used in JSON-RPC. Ignores all but 'Content-Length:', which it returns the value of.
size_t readHeaders(FILE* inputStream) {
    std::array<char, 256> headerBuf;
    size_t contentLength = 0;

    while (!feof(inputStream)) {
        if (ferror(inputStream)) {
            // LOGGING
            return 0;
        }

        fgets(headerBuf.data(), 256, inputStream);
        std::string headerLine(headerBuf.data());

        if (headerLine.size() == 0) {
            continue;
        }

        // Header lines longer than 256 characters are parse errors for now.
        if (headerLine.back() != '\n') {
            // LOGGING
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

    // LOGGING - EOF encountered during header parsing, this is an error condition
    return 0;
}

// Serialize and send a JSON-RPC message to the client.
void sendMessage(rapidjson::Document& document, FILE* outputStream) {
    // Serialize the document to memory to compute its length, needed for the JSON-RPC header.
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    std::string output = fmt::format("Content-Length: {}\r\n\r\n{}", buffer.GetSize(), buffer.GetString());
    fwrite(output.data(), 1, output.size(), outputStream);
}

bool handleMethod(const std::string& methodName, std::optional<server::lsp::ID> id, rapidjson::Document& document) {
    auto method = server::lsp::getMethodNamed(methodName.data(), methodName.size());

    switch(method) {
    case server::lsp::Method::kNotFound:
        return false;
    case server::lsp::Method::kInitialize:
        break;
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
}

bool handleResponse(lsp::ID /* id */, rapidjson::Document& document) {

}

}

namespace server {

JSONTransport::JSONTransport(FILE* inputStream, FILE* outputStream):
    m_inputStream(inputStream),
    m_outputStream(outputStream) {}

int JSONTransport::runLoop() {
    std::vector<char> json;

    while (!feof(m_inputStream)) {
        size_t contentLength = readHeaders(m_inputStream);
        if (contentLength == 0) {
            return -1;
        }
        // JSON objects longer than a megabyte fail to parse.
        if (contentLength > 1024 * 1024) {
            // LOGGING
            return -1;
        }

        json.resize(contentLength + 1);
        size_t bytesRead = fread(json.data(), 1, contentLength, m_inputStream);
        if (bytesRead == 0 || ferror(m_inputStream)) {
            // LOGGING
            return -1;
        }

        rapidjson::Document document;
        rapidjson::ParseResult parseResult = document.Parse(json.data());
        if (!parseResult) {
            // LOGGING
            continue;
        }

        // Validate some basic properties of the received JSON-RPC object
        if (!document.IsObject() ||
            !document.HasMember("jsonrpc") ||
            !document["jsonrpc"].IsString() ||
            std::strcmp(document["jsonprc"].GetString(), "2.0") != 0) {
            // LOGGING
            continue;
        }

        std::optional<lsp::ID> id;
        if (document.HasMember("id")) {
            if (document["id"].IsString()) {
                id = std::string(document["id"].GetString());
            } else if (document["id"].IsNumber()) {
                id = document["id"].GetInt64();
            } else {
                // malformed ID
                continue;
            }
        }
        std::optional<std::string> method;
        if (document.HasMember("method")) {
            if (document["method"].IsString()) {
                method = std::string(document["method"].GetString());
            } else {
                // malformed method
                continue;
            }
        }

        //           | no ID        | ID       |
        // ----------+--------------+----------+
        // no method | *invalid*    | response |
        // method    | notification | request  |
        if (!method) {
            if (!id) {
                // LOGGING invalid block
                continue;
            }
            handleResponse(id.value(), document);
        } else {
            handleMethod(method.value(), id, document);
        }

        sendMessage(document, m_outputStream);
    }

    // EOF means normal exit of loop, return.
    return 0;
}

} // namespace server