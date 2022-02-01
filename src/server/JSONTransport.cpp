#include "server/JSONTransport.hpp"

#include "hadron/Block.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/Frame.hpp"
#include "hadron/HIR.hpp"
#include "hadron/internal/BuildInfo.hpp"
#include "hadron/LifetimeInterval.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/OpcodeIterator.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Scope.hpp"
#include "hadron/VirtualJIT.hpp"
#include "server/HadronServer.hpp"
#include "server/LSPMethods.hpp"

#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "spdlog/spdlog.h"

#include <array>
#include <cerrno>
#include <string.h>
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
    // Fills out the ServerCapabilities structure before sending to client.
    void sendInitializeResult(std::optional<lsp::ID> id);
    void sendSemanticTokens(const std::vector<hadron::Token>& tokens);
    void sendCompilationDiagnostics(lsp::ID id, const std::vector<CompilationUnit>& compilationUnits);

private:
    size_t readHeaders();
    void sendMessage(rapidjson::Document& document);
    bool handleMethod(const std::string& methodName, std::optional<lsp::ID> id, rapidjson::Document& document);
    bool handleResponse(lsp::ID id, rapidjson::Document& document);
    void encodeId(std::optional<lsp::ID> id, rapidjson::Document& document);

    void handleInitialize(std::optional<lsp::ID> id, const rapidjson::GenericObject<false, rapidjson::Value>& params);
    void serializeParseNode(const hadron::parse::Node* node, rapidjson::Document& document,
            std::vector<rapidjson::Pointer::Token>& path, int& serial);
    void serializeSlot(hadron::Slot slot, rapidjson::Value& target, rapidjson::Document& document);
    void serializeFrame(const hadron::Frame* frame, rapidjson::Value& jsonFrame,
            rapidjson::Document& document);
    void serializeScope(const hadron::Scope* scope, int& scopeSerial, rapidjson::Value& jsonScope,
            rapidjson::Document& document);
    void serializeLinearBlock(const hadron::LinearBlock* linearBlock, rapidjson::Value& jsonBlock,
            rapidjson::Document& document);
    void serializeJIT(const int8_t* byteCode, size_t byteCodeSize, rapidjson::Value& jsonJIT,
            rapidjson::Document& document);
    void serializeHIR(const hadron::hir::HIR* hir, rapidjson::Value& jsonHIR, rapidjson::Document& document);
    void serializeValue(hadron::Value value, rapidjson::Value& jsonValue, rapidjson::Document& document);
    void serializeLifetimeIntervals(const std::vector<std::vector<hadron::LtIRef>>& lifetimeIntervals,
            rapidjson::Value& jsonIntervals, rapidjson::Document& document);

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
        while (bytesRead < contentLength) {
            size_t read = fread(json.data() + bytesRead, 1, contentLength - bytesRead, m_inputStream);
            if (read == 0 || ferror(m_inputStream)) {
                SPDLOG_CRITICAL("Input read failure.");
                return -1;
            }
            bytesRead += read;
        }
        json[contentLength] = '\0';
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

        // LSP terminology:
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
    rapidjson::Value semanticTokensProvider;
    semanticTokensProvider.SetObject();
    semanticTokensProvider.AddMember("range", rapidjson::Value(false), document.GetAllocator());
    semanticTokensProvider.AddMember("full", rapidjson::Value(true), document.GetAllocator());
    rapidjson::Value semanticTokensLegend;
    semanticTokensLegend.SetObject();
    rapidjson::Value tokenTypes;
    tokenTypes.SetArray();
    tokenTypes.PushBack(rapidjson::Value("empty"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("interpret"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("literal"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("primitive"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("plus"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("minus"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("asterisk"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("assign"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("lessThan"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("greaterThan"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("pipe"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("readWriteVar"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("leftArrow"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("binop"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("keyword"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("openParen"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("closeParen"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("openCurly"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("closeCurly"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("openSquare"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("closeSquare"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("comma"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("semicolon"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("colon"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("caret"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("tilde"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("hash"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("grave"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("var"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("arg"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("const"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("classVar"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("identifier"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("className"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("dot"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("dotdot"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("ellipses"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("curryArgument"), document.GetAllocator());
    tokenTypes.PushBack(rapidjson::Value("if"), document.GetAllocator());
    semanticTokensLegend.AddMember("tokenTypes", tokenTypes, document.GetAllocator());
    rapidjson::Value tokenModifiers;
    tokenModifiers.SetArray();
    semanticTokensLegend.AddMember("tokenModifiers", tokenModifiers, document.GetAllocator());
    semanticTokensProvider.AddMember("legend", semanticTokensLegend, document.GetAllocator());
    capabilities.AddMember("semanticTokensProvider", semanticTokensProvider, document.GetAllocator());
    result.AddMember("capabilities", capabilities, document.GetAllocator());
    rapidjson::Value serverInfo;
    serverInfo.SetObject();
    serverInfo.AddMember("name", rapidjson::Value("hlangd"), document.GetAllocator());
    serverInfo.AddMember("version", rapidjson::Value(hadron::kHadronVersion, document.GetAllocator()),
            document.GetAllocator());
    result.AddMember("serverInfo", serverInfo, document.GetAllocator());
    document.AddMember("result", result, document.GetAllocator());
    sendMessage(document);
}

void JSONTransport::JSONTransportImpl::sendSemanticTokens(const std::vector<hadron::Token>& tokens) {
    rapidjson::Document document;
    document.SetObject();
    document.AddMember("jsonrpc", rapidjson::Value("2.0"), document.GetAllocator());
    rapidjson::Value data;
    data.SetArray();
    size_t lineNumber = 0;
    size_t characterNumber = 0;
    for (auto token : tokens) {
        // First element is deltaLine, line number relative to previous token.
        data.PushBack(static_cast<unsigned>(token.location.lineNumber - lineNumber), document.GetAllocator());
        if (token.location.lineNumber != lineNumber) {
            lineNumber = token.location.lineNumber;
            characterNumber = 0;
        }
        // Second element is deltaStart, character number relative to previous token (if on same line) or 0.
        data.PushBack(static_cast<unsigned>(token.location.characterNumber - characterNumber), document.GetAllocator());
        characterNumber = token.location.characterNumber;
        // Third element is length, length of the token.
        data.PushBack(static_cast<unsigned>(token.range.size()), document.GetAllocator());
        // Fourth element is the token type, a raw encoding of the enum value.
        data.PushBack(token.name, document.GetAllocator());
        // Fifth element is the token modifiers, zero for now.
        data.PushBack(0U, document.GetAllocator());
    }
    rapidjson::Value result;
    result.SetObject();
    result.AddMember("data", data, document.GetAllocator());
    document.AddMember("result", result, document.GetAllocator());
    sendMessage(document);
}

void JSONTransport::JSONTransportImpl::sendCompilationDiagnostics(lsp::ID id,
        const std::vector<CompilationUnit>& compilationUnits) {
    rapidjson::Document document;
    document.SetObject();
    document.AddMember("jsonrpc", rapidjson::Value("2.0"), document.GetAllocator());
    encodeId(id, document);

    rapidjson::Value jsonUnits;
    jsonUnits.SetArray();

    int serial = 0;
    for (const auto& unit : compilationUnits) {
        rapidjson::Value jsonUnit;
        jsonUnit.SetObject();

        SPDLOG_TRACE("Serializing compilation unit {}", unit.name);
        rapidjson::Value nameString;
        nameString.SetString(unit.name.data(), document.GetAllocator());
        jsonUnit.AddMember("name", nameString, document.GetAllocator());

        SPDLOG_TRACE("Serializing root frame for {}", unit.name);
        rapidjson::Value rootFrame;
        serializeFrame(unit.frame.get(), rootFrame, document);
        jsonUnit.AddMember("rootFrame", rootFrame, document.GetAllocator());

        SPDLOG_TRACE("Serializing linearBlock for {}", unit.name);
        rapidjson::Value jsonBlock;
        serializeLinearBlock(unit.linearBlock.get(), jsonBlock, document);
        jsonUnit.AddMember("linearBlock", jsonBlock, document.GetAllocator());

        SPDLOG_TRACE("Serializing machine code for {}", unit.name);
        rapidjson::Value jsonJIT;
        serializeJIT(unit.byteCode.get(), unit.byteCodeSize, jsonJIT, document);
        jsonUnit.AddMember("machineCode", jsonJIT, document.GetAllocator());

        jsonUnits.PushBack(jsonUnit, document.GetAllocator());
    }

    rapidjson::Value result;
    result.SetObject();
    result.AddMember("compilationUnits", jsonUnits, document.GetAllocator());
    document.AddMember("result", result, document.GetAllocator());

    // rapidjson::Pointer seems to want to anchor on existing nodes, so we add the parse tree to existing entries
    // in the compilationUnits array after its already been added to the DOM. Note that due to move semantics
    // in AddMember() |jsonUnits| is now invalid.
    for (size_t i = 0; i < compilationUnits.size(); ++i) {
        const auto& unit = compilationUnits[i];
        auto index = fmt::format("{}", i);
        std::vector<rapidjson::Pointer::Token> path({
                {"result", sizeof("result") - 1, rapidjson::kPointerInvalidIndex},
                {"compilationUnits", sizeof("compilationUnits") - 1, rapidjson::kPointerInvalidIndex},
                {index.data(), static_cast<rapidjson::SizeType>(index.size()) - 1,
                        static_cast<rapidjson::SizeType>(i)},
                {"parseTree", sizeof("parseTree") - 1, rapidjson::kPointerInvalidIndex}
            });
        serial = 0;
        serializeParseNode(unit.blockNode, document, path, serial);
    }

    sendMessage(document);
}

// Parses the HTTP-style headers used in JSON-RPC. Ignores all but 'Content-Length:', which it returns the value of.
size_t JSONTransport::JSONTransportImpl::readHeaders() {
    std::array<char, 256> headerBuf;
    size_t contentLength = 0;

    while (!feof(m_inputStream)) {
        if (int fileError = ferror(m_inputStream)) {
            if (fileError == EINTR) {
                errno = 0;
            } else {
                SPDLOG_ERROR("File error on input stream while reading headers: {}.", strerror(fileError));
                return 0;
            }
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
    std::string output = fmt::format("Content-Length: {}\r\n\r\n{}\n", buffer.GetSize(), buffer.GetString());
    std::fwrite(output.data(), 1, output.size(), m_outputStream);
    std::fflush(m_outputStream);
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

    // Look for a params object if present, as many methods call for that.
    rapidjson::Value* params = rapidjson::Pointer("/params").Get(document);
    if (params && !params->IsObject()) {
        params = nullptr;
    }

    switch(method) {
    case server::lsp::Method::kNotFound:
        sendErrorResponse(id, ErrorCode::kMethodNotFound, fmt::format("Failed to match method '{}' to supported name.",
            methodName));
        return false;
    case server::lsp::Method::kInitialize: {
        SPDLOG_TRACE("handleInitialize");
        if (!params) {
            sendErrorResponse(id, ErrorCode::kInvalidParams, "Absent or malformed params key in 'initialize' method.");
            return false;
        }
        handleInitialize(id, params->GetObject());
    } break;
    case server::lsp::Method::kInitialized:
        SPDLOG_TRACE("initalized");
        break;
    case server::lsp::Method::kShutdown:
        break;
    case server::lsp::Method::kExit:
        break;
    case server::lsp::Method::kLogTrace:
        break;
    case server::lsp::Method::kSetTrace:
        break;
    case server::lsp::Method::kSemanticTokensFull: {
        // Assumes params->textDocument->uri exists and is valid.
        SPDLOG_TRACE("semanticTokensFull on file: {}", (*params)["textDocument"]["uri"].GetString());
        m_server->semanticTokensFull((*params)["textDocument"]["uri"].GetString());
    } break;
    case server::lsp::Method::kHadronCompilationDiagnostics: {
        // Assumes params->textDocument->uri exists and is valid.
        SPDLOG_TRACE("compilationDiagnostics on file: {}", (*params)["textDocument"]["uri"].GetString());
        m_server->hadronCompilationDiagnostics(*id, (*params)["textDocument"]["uri"].GetString());
    } break;
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

void JSONTransport::JSONTransportImpl::serializeParseNode(const hadron::parse::Node* node,
        rapidjson::Document& document, std::vector<rapidjson::Pointer::Token>& path, int& serial) {
    rapidjson::Value& jsonNode = rapidjson::CreateValueByPointer(document,
            rapidjson::Pointer(path.data(), path.size()), document.GetAllocator());
    if (!node) {
        return;
    }
    jsonNode.SetObject();
    jsonNode.AddMember("tokenIndex", rapidjson::Value(static_cast<uint64_t>(node->tokenIndex)),
            document.GetAllocator());
    int nodeSerial = serial;
    ++serial;
    jsonNode.AddMember("serial", rapidjson::Value(nodeSerial), document.GetAllocator());
    switch(node->nodeType) {
    case hadron::parse::NodeType::kEmpty:
        jsonNode.AddMember("nodeType", rapidjson::Value("Empty"), document.GetAllocator());
        break;
    case hadron::parse::NodeType::kVarDef: {
        const auto varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("VarDef"), document.GetAllocator());
        jsonNode.AddMember("hasReadAccessor", rapidjson::Value(varDef->hasReadAccessor), document.GetAllocator());
        jsonNode.AddMember("hasWriteAccessor", rapidjson::Value(varDef->hasWriteAccessor), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"initialValue", sizeof("initialValue") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(varDef->initialValue.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kVarList: {
        const auto varList = reinterpret_cast<const hadron::parse::VarListNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("VarList"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"definitions", sizeof("definitions") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(varList->definitions.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kArgList: {
        const auto argList = reinterpret_cast<const hadron::parse::ArgListNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("ArgList"), document.GetAllocator());
        if (argList->varArgsNameIndex) {
            jsonNode.AddMember("varArgsNameIndex",
                rapidjson::Value(static_cast<uint64_t>(*(argList->varArgsNameIndex))), document.GetAllocator());
        } else {
            jsonNode.AddMember("varArgsNameIndex", rapidjson::Value(), document.GetAllocator());
        }
        path.emplace_back(rapidjson::Pointer::Token{"varList", sizeof("varList") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(argList->varList.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kMethod: {
        const auto method = reinterpret_cast<const hadron::parse::MethodNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Method"), document.GetAllocator());
        jsonNode.AddMember("isClassMethod", rapidjson::Value(method->isClassMethod), document.GetAllocator());
        if (method->primitiveIndex) {
            jsonNode.AddMember("primitiveIndex", rapidjson::Value(static_cast<uint64_t>(*(method->primitiveIndex))),
                document.GetAllocator());
        } else {
            jsonNode.AddMember("primitiveIndex", rapidjson::Value(), document.GetAllocator());
        }
        path.emplace_back(rapidjson::Pointer::Token{"body", sizeof("body") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(method->body.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kClassExt: {
        const auto classExt = reinterpret_cast<const hadron::parse::ClassExtNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("ClassExt"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"methods", sizeof("methods") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(classExt->methods.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kClass: {
        const auto classNode = reinterpret_cast<const hadron::parse::ClassNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Class"), document.GetAllocator());
        if (classNode->superClassNameIndex) {
            jsonNode.AddMember("superClassNameIndex", rapidjson::Value(
                static_cast<uint64_t>(*(classNode->superClassNameIndex))), document.GetAllocator());
        } else {
            jsonNode.AddMember("superClassNameIndex", rapidjson::Value(), document.GetAllocator());
        }
        if (classNode->optionalNameIndex) {
            jsonNode.AddMember("optionalNameIndex", rapidjson::Value(
                static_cast<uint64_t>(*(classNode->optionalNameIndex))), document.GetAllocator());
        } else {
            jsonNode.AddMember("optionalNameIndex", rapidjson::Value(), document.GetAllocator());
        }
        path.emplace_back(rapidjson::Pointer::Token{"variables", sizeof("variables") - 1,
            rapidjson::kPointerInvalidIndex});
        serializeParseNode(classNode->variables.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"methods", sizeof("methods") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(classNode->methods.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kReturn: {
        const auto retNode = reinterpret_cast<const hadron::parse::ReturnNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("ReturnNode"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"valueExpr", sizeof("valueExpr") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(retNode->valueExpr.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kList: {
        const auto list = reinterpret_cast<const hadron::parse::ListNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("List"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"elements", sizeof("elements") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(list->elements.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kDictionary: {
        const auto dict = reinterpret_cast<const hadron::parse::DictionaryNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Dictionary"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"elements", sizeof("elements") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(dict->elements.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kBlock: {
        const auto block = reinterpret_cast<const hadron::parse::BlockNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Block"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"arguments", sizeof("arguments") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(block->arguments.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"variables", sizeof("variables") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(block->variables.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"body", sizeof("body") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(block->body.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kLiteral: {
        const auto literal = reinterpret_cast<const hadron::parse::LiteralNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Literal"), document.GetAllocator());
        rapidjson::Value value;
        serializeSlot(literal->value, value, document);
        jsonNode.AddMember("value", value, document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"blockLiteral", sizeof("blockLiteral") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(literal->blockLiteral.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kName: {
        const auto name = reinterpret_cast<const hadron::parse::NameNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Name"), document.GetAllocator());
        jsonNode.AddMember("isGlobal", rapidjson::Value(name->isGlobal), document.GetAllocator());
    } break;
    case hadron::parse::NodeType::kExprSeq: {
        const auto exprSeq = reinterpret_cast<const hadron::parse::ExprSeqNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("ExprSeq"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"expr", sizeof("expr") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(exprSeq->expr.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kAssign: {
        const auto assign = reinterpret_cast<const hadron::parse::AssignNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Assign"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"name", sizeof("name") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(assign->name.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"value", sizeof("value") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(assign->value.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kSetter: {
        const auto setter = reinterpret_cast<const hadron::parse::SetterNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Setter"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"target", sizeof("target") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(setter->target.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"value", sizeof("value") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(setter->value.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kKeyValue: {
        const auto keyValue = reinterpret_cast<const hadron::parse::KeyValueNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("KeyValue"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"value", sizeof("value") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(keyValue->value.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kCall: {
        const auto call = reinterpret_cast<const hadron::parse::CallNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Call"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"target", sizeof("target") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(call->target.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"arguments", sizeof("arguments") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(call->arguments.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"keywordArguments", sizeof("keywordArguments") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(call->keywordArguments.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kBinopCall: {
        const auto binop = reinterpret_cast<const hadron::parse::BinopCallNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("BinopCall"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"leftHand", sizeof("leftHand") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(binop->leftHand.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"rightHand", sizeof("rightHand") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(binop->rightHand.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"adverb", sizeof("adverb") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(binop->adverb.get(), document, path, serial);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kPerformList: {
        // TODO
    } break;
    case hadron::parse::NodeType::kNumericSeries: {
        // TODO
    } break;
    case hadron::parse::NodeType::kCurryArgument: {
        // TODO
    } break;
    case hadron::parse::NodeType::kArrayRead: {
        // TODO
    } break;
    case hadron::parse::NodeType::kArrayWrite: {
        // TODO
    } break;
    case hadron::parse::NodeType::kCopySeries: {
        // TODO
    } break;
    case hadron::parse::NodeType::kNew: {
        // TODO
    } break;
    case hadron::parse::NodeType::kSeries: {
        // TODO
    } break;
    case hadron::parse::NodeType::kSeriesIter: {
        // TODO
    } break;
    case hadron::parse::NodeType::kLiteralList: {
        // TODO
    } break;
    case hadron::parse::NodeType::kLiteralDict: {
        // TODO
    } break;
    case hadron::parse::NodeType::kMultiAssignVars: {
        // TODO
    } break;
    case hadron::parse::NodeType::kMultiAssign: {
        // TODO
    } break;
    case hadron::parse::NodeType::kIf: {
        const auto ifNode = reinterpret_cast<const hadron::parse::IfNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("If"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"condition", sizeof("condition") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(ifNode->condition.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"trueBlock", sizeof("trueBlock") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(ifNode->trueBlock.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"falseBlock", sizeof("falseBlock") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(ifNode->falseBlock.get(), document, path, serial);
        path.pop_back();
    } break;
    }

    path.emplace_back(rapidjson::Pointer::Token{"next", sizeof("next") - 1, rapidjson::kPointerInvalidIndex});
    serializeParseNode(node->next.get(), document, path, serial);
    path.pop_back();
}

void JSONTransport::JSONTransportImpl::serializeSlot(hadron::Slot slot, rapidjson::Value& target,
        rapidjson::Document& document) {
    target.SetObject();
    switch (slot.getType()) {
    case hadron::Type::kNil:
        target.AddMember("type", rapidjson::Value("nil"), document.GetAllocator());
        target.AddMember("value", rapidjson::Value("nil"), document.GetAllocator());
        break;
    case hadron::Type::kInteger:
        target.AddMember("type", rapidjson::Value("integer"), document.GetAllocator());
        target.AddMember("value", rapidjson::Value(slot.getInt32()), document.GetAllocator());
        break;
    case hadron::Type::kFloat:
        target.AddMember("type", rapidjson::Value("float"), document.GetAllocator());
        target.AddMember("value", rapidjson::Value(slot.getFloat()), document.GetAllocator());
        break;
    case hadron::Type::kBoolean:
        target.AddMember("type", rapidjson::Value("boolean"), document.GetAllocator());
        target.AddMember("value", rapidjson::Value(slot.getBool()), document.GetAllocator());
        break;
    case hadron::Type::kSymbol:
        target.AddMember("type", rapidjson::Value("symbol"), document.GetAllocator());
        break;
    default:
        target.AddMember("type", rapidjson::Value("unknown"), document.GetAllocator());
        break;
    }
}

void JSONTransport::JSONTransportImpl::serializeFrame(const hadron::Frame* frame, rapidjson::Value& jsonFrame,
        rapidjson::Document& document) {
    jsonFrame.SetObject();
    // TODO: argumentOrder
    rapidjson::Value rootScope;
    int scopeSerial = 1;
    serializeScope(frame->rootScope.get(), scopeSerial, rootScope, document);
    jsonFrame.AddMember("rootScope", rootScope, document.GetAllocator());
    jsonFrame.AddMember("numberOfValues", rapidjson::Value(static_cast<uint64_t>(frame->numberOfValues)),
            document.GetAllocator());
    jsonFrame.AddMember("numberOfBlocks", rapidjson::Value(frame->numberOfBlocks), document.GetAllocator());
}

void JSONTransport::JSONTransportImpl::serializeScope(const hadron::Scope* scope, int& scopeSerial,
        rapidjson::Value& jsonScope, rapidjson::Document& document) {
    jsonScope.SetObject();
    int serial = scopeSerial;
    ++scopeSerial;
    jsonScope.AddMember("scopeSerial", rapidjson::Value(serial), document.GetAllocator());
    rapidjson::Value blocks;
    blocks.SetArray();
    for (const auto& block : scope->blocks) {
        rapidjson::Value jsonBlock;
        jsonBlock.SetObject();
        jsonBlock.AddMember("number", rapidjson::Value(block->number), document.GetAllocator());
        rapidjson::Value predecessors;
        predecessors.SetArray();
        for (const auto pred : block->predecessors) {
            predecessors.PushBack(rapidjson::Value(pred->number), document.GetAllocator());
        }
        jsonBlock.AddMember("predecessors", predecessors, document.GetAllocator());
        rapidjson::Value successors;
        successors.SetArray();
        for (const auto succ : block->successors) {
            successors.PushBack(rapidjson::Value(succ->number), document.GetAllocator());
        }
        jsonBlock.AddMember("successors", successors, document.GetAllocator());
        rapidjson::Value phis;
        phis.SetArray();
        for (const auto& phi : block->phis) {
            rapidjson::Value jsonPhi;
            serializeHIR(phi.get(), jsonPhi, document);
            phis.PushBack(jsonPhi, document.GetAllocator());
        }
        jsonBlock.AddMember("phis", phis, document.GetAllocator());
        rapidjson::Value statements;
        statements.SetArray();
        for (const auto& hir : block->statements) {
            rapidjson::Value jsonHIR;
            serializeHIR(hir.get(), jsonHIR, document);
            statements.PushBack(jsonHIR, document.GetAllocator());
        }
        jsonBlock.AddMember("statements", statements, document.GetAllocator());
        blocks.PushBack(jsonBlock, document.GetAllocator());
    }
    jsonScope.AddMember("blocks", blocks, document.GetAllocator());
    rapidjson::Value subScopes;
    subScopes.SetArray();
    for (const auto& subScope : scope->subScopes) {
        rapidjson::Value jsonSubScope;
        serializeScope(subScope.get(), scopeSerial, jsonSubScope, document);
        subScopes.PushBack(jsonSubScope, document.GetAllocator());
    }
    jsonScope.AddMember("subScopes", subScopes, document.GetAllocator());
}

void JSONTransport::JSONTransportImpl::serializeLinearBlock(const hadron::LinearBlock* linearBlock,
        rapidjson::Value& jsonBlock, rapidjson::Document& document) {
    jsonBlock.SetObject();
    rapidjson::Value instructions;
    instructions.SetArray();
    for (const auto& hir : linearBlock->instructions) {
        rapidjson::Value jsonHIR;
        serializeHIR(hir.get(), jsonHIR, document);
        instructions.PushBack(jsonHIR, document.GetAllocator());
    }
    jsonBlock.AddMember("instructions", instructions, document.GetAllocator());

    rapidjson::Value blockOrder;
    blockOrder.SetArray();
    for (auto number : linearBlock->blockOrder) {
        blockOrder.PushBack(rapidjson::Value(number), document.GetAllocator());
    }
    jsonBlock.AddMember("blockOrder", blockOrder, document.GetAllocator());

    rapidjson::Value blockRanges;
    blockRanges.SetArray();
    for (auto range : linearBlock->blockRanges) {
        rapidjson::Value jsonRange;
        jsonRange.SetArray();
        jsonRange.PushBack(rapidjson::Value(static_cast<uint64_t>(range.first)), document.GetAllocator());
        jsonRange.PushBack(rapidjson::Value(static_cast<uint64_t>(range.second)), document.GetAllocator());
        blockRanges.PushBack(jsonRange, document.GetAllocator());
    }
    jsonBlock.AddMember("blockRanges", blockRanges, document.GetAllocator());

    rapidjson::Value valueLifetimes;
    serializeLifetimeIntervals(linearBlock->valueLifetimes, valueLifetimes, document);
    jsonBlock.AddMember("valueLifetimes", valueLifetimes, document.GetAllocator());

    jsonBlock.AddMember("numberOfSpillSlots", rapidjson::Value(static_cast<uint64_t>(linearBlock->numberOfSpillSlots)),
            document.GetAllocator());
}

void JSONTransport::JSONTransportImpl::serializeJIT(const int8_t* byteCode, size_t byteCodeSize,
        rapidjson::Value& jsonJIT, rapidjson::Document& document) {
    jsonJIT.SetObject();
    rapidjson::Value instructions;
    instructions.SetArray();
    hadron::OpcodeReadIterator it(byteCode, byteCodeSize);
    while (it.getSize() < byteCodeSize) {
        rapidjson::Value opcode;
        opcode.SetArray();
        // Save the address of the opcode first.
        opcode.PushBack(static_cast<uint64_t>(it.getSize()), document.GetAllocator());
        switch(it.peek()) {
        case hadron::Opcode::kInvalid: {
            assert(false);
        } break;
        case hadron::Opcode::kAddr: {
            opcode.PushBack("addr", document.GetAllocator());
            hadron::JIT::Reg target, a, b;
            it.addr(target, a, b);
            opcode.PushBack(target, document.GetAllocator());
            opcode.PushBack(a, document.GetAllocator());
            opcode.PushBack(b, document.GetAllocator());
        } break;
        case hadron::Opcode::kAddi: {
            opcode.PushBack("addi", document.GetAllocator());
            hadron::JIT::Reg target, a;
            hadron::Word b;
            it.addi(target, a, b);
            opcode.PushBack(target, document.GetAllocator());
            opcode.PushBack(a, document.GetAllocator());
            opcode.PushBack(b, document.GetAllocator());
        } break;
        case hadron::Opcode::kAndi: {
            opcode.PushBack("andi", document.GetAllocator());
            hadron::JIT::Reg target, a;
            hadron::UWord b;
            it.andi(target, a, b);
            opcode.PushBack(target, document.GetAllocator());
            opcode.PushBack(a, document.GetAllocator());
            opcode.PushBack(b, document.GetAllocator());
        } break;
        case hadron::Opcode::kOri: {
            opcode.PushBack("ori", document.GetAllocator());
            hadron::JIT::Reg target, a;
            hadron::UWord b;
            it.ori(target, a, b);
            opcode.PushBack(target, document.GetAllocator());
            opcode.PushBack(a, document.GetAllocator());
            opcode.PushBack(b, document.GetAllocator());
        } break;
        case hadron::Opcode::kXorr: {
            opcode.PushBack("xorr", document.GetAllocator());
            hadron::JIT::Reg target, a, b;
            it.xorr(target, a, b);
            opcode.PushBack(target, document.GetAllocator());
            opcode.PushBack(a, document.GetAllocator());
            opcode.PushBack(b, document.GetAllocator());
        } break;
        case hadron::Opcode::kMovr: {
            opcode.PushBack("movr", document.GetAllocator());
            hadron::JIT::Reg target, value;
            it.movr(target, value);
            opcode.PushBack(target, document.GetAllocator());
            opcode.PushBack(value, document.GetAllocator());
        } break;
        case hadron::Opcode::kMovi: {
            opcode.PushBack("movi", document.GetAllocator());
            hadron::JIT::Reg target;
            hadron::Word value;
            it.movi(target, value);
            opcode.PushBack(target, document.GetAllocator());
            opcode.PushBack(value, document.GetAllocator());
        } break;
        case hadron::Opcode::kBgei: {
            opcode.PushBack("bgei", document.GetAllocator());
            hadron::JIT::Reg a;
            hadron::Word b;
            const int8_t* address;
            it.bgei(a, b, address);
            opcode.PushBack(a, document.GetAllocator());
            opcode.PushBack(b, document.GetAllocator());
            opcode.PushBack(static_cast<int>(address - byteCode), document.GetAllocator());
        } break;
        case hadron::Opcode::kBeqi: {
            opcode.PushBack("beqi", document.GetAllocator());
            hadron::JIT::Reg a;
            hadron::Word b;
            const int8_t* address = nullptr;
            it.beqi(a, b, address);
            opcode.PushBack(a, document.GetAllocator());
            opcode.PushBack(b, document.GetAllocator());
            opcode.PushBack(static_cast<int>(address - byteCode), document.GetAllocator());
        } break;
        case hadron::Opcode::kJmp: {
            opcode.PushBack("jmp", document.GetAllocator());
            const int8_t* address = nullptr;
            it.jmp(address);
            opcode.PushBack(static_cast<int>(address - byteCode), document.GetAllocator());
        } break;
        case hadron::Opcode::kJmpr: {
            opcode.PushBack("jmpr", document.GetAllocator());
            hadron::JIT::Reg r;
            it.jmpr(r);
            opcode.PushBack(r, document.GetAllocator());
        } break;
        case hadron::Opcode::kJmpi: {
            opcode.PushBack("jmpi", document.GetAllocator());
            hadron::UWord location;
            it.jmpi(location);
            opcode.PushBack(location, document.GetAllocator());
        } break;
        case hadron::Opcode::kLdrL: {
            opcode.PushBack("ldr_l", document.GetAllocator());
            hadron::JIT::Reg target, address;
            it.ldr_l(target, address);
            opcode.PushBack(target, document.GetAllocator());
            opcode.PushBack(address, document.GetAllocator());
        } break;
        case hadron::Opcode::kLdxiW: {
            opcode.PushBack("ldxi_w", document.GetAllocator());
            hadron::JIT::Reg target, address;
            int offset;
            it.ldxi_w(target, address, offset);
            opcode.PushBack(target, document.GetAllocator());
            opcode.PushBack(address, document.GetAllocator());
            opcode.PushBack(offset, document.GetAllocator());
        } break;
        case hadron::Opcode::kLdxiI: {
            opcode.PushBack("ldxi_i", document.GetAllocator());
            hadron::JIT::Reg target, address;
            int offset;
            it.ldxi_i(target, address, offset);
            opcode.PushBack(target, document.GetAllocator());
            opcode.PushBack(address, document.GetAllocator());
            opcode.PushBack(offset, document.GetAllocator());
        } break;
        case hadron::Opcode::kLdxiL: {
            opcode.PushBack("ldxi_l", document.GetAllocator());
            hadron::JIT::Reg target, address;
            int offset;
            it.ldxi_l(target, address, offset);
            opcode.PushBack(target, document.GetAllocator());
            opcode.PushBack(address, document.GetAllocator());
            opcode.PushBack(offset, document.GetAllocator());
        } break;
        case hadron::Opcode::kStrI: {
            opcode.PushBack("str_i", document.GetAllocator());
            hadron::JIT::Reg address, value;
            it.str_i(address, value);
            opcode.PushBack(address, document.GetAllocator());
            opcode.PushBack(value, document.GetAllocator());
        } break;
        case hadron::Opcode::kStrL: {
            opcode.PushBack("str_l", document.GetAllocator());
            hadron::JIT::Reg address, value;
            it.str_l(address, value);
            opcode.PushBack(address, document.GetAllocator());
            opcode.PushBack(value, document.GetAllocator());
        } break;
        case hadron::Opcode::kStxiW: {
            opcode.PushBack("stxi_w", document.GetAllocator());
            int offset;
            hadron::JIT::Reg address, value;
            it.stxi_w(offset, address, value);
            opcode.PushBack(offset, document.GetAllocator());
            opcode.PushBack(address, document.GetAllocator());
            opcode.PushBack(value, document.GetAllocator());
        } break;
        case hadron::Opcode::kStxiI: {
            opcode.PushBack("stxi_i", document.GetAllocator());
            int offset;
            hadron::JIT::Reg address, value;
            it.stxi_i(offset, address, value);
            opcode.PushBack(offset, document.GetAllocator());
            opcode.PushBack(address, document.GetAllocator());
            opcode.PushBack(value, document.GetAllocator());
        } break;
        case hadron::Opcode::kStxiL: {
            opcode.PushBack("stxi_l", document.GetAllocator());
            int offset;
            hadron::JIT::Reg address, value;
            it.stxi_l(offset, address, value);
            opcode.PushBack(offset, document.GetAllocator());
            opcode.PushBack(address, document.GetAllocator());
            opcode.PushBack(value, document.GetAllocator());
        } break;
        case hadron::Opcode::kRet: {
            opcode.PushBack("ret", document.GetAllocator());
            it.ret();
        } break;
        case hadron::Opcode::kRetr: {
            opcode.PushBack("retr", document.GetAllocator());
            hadron::JIT::Reg r;
            it.retr(r);
            opcode.PushBack(r, document.GetAllocator());
        } break;
        case hadron::Opcode::kReti: {
            opcode.PushBack("reti", document.GetAllocator());
            int value;
            it.reti(value);
            opcode.PushBack(value, document.GetAllocator());
        } break;
        case hadron::Opcode::kLabel: {
            assert(false); // we currently don't serialize labels
            opcode.PushBack("label", document.GetAllocator());
        } break;
        case hadron::Opcode::kAddress: {
            assert(false); // we currently don't serialize addresses
            opcode.PushBack("address", document.GetAllocator());
        } break;
        case hadron::Opcode::kPatchHere: {
            assert(false); // not serialized
            opcode.PushBack("patch_here", document.GetAllocator());
        } break;
        case hadron::Opcode::kPatchThere: {
            assert(false); // not serialized
            opcode.PushBack("patch_there", document.GetAllocator());
        } break;
        }
        instructions.PushBack(opcode, document.GetAllocator());
    }
    jsonJIT.AddMember("instructions", instructions, document.GetAllocator());
}


void JSONTransport::JSONTransportImpl::serializeHIR(const hadron::hir::HIR* hir, rapidjson::Value& jsonHIR,
        rapidjson::Document& document) {
    jsonHIR.SetObject();
    rapidjson::Value value;
    serializeValue(hir->value, value, document);
    jsonHIR.AddMember("value", value, document.GetAllocator());
    rapidjson::Value reads;
    reads.SetArray();
    for (auto read : hir->reads) {
        rapidjson::Value jsonRead;
        serializeValue(read, jsonRead, document);
        reads.PushBack(jsonRead, document.GetAllocator());
    }
    jsonHIR.AddMember("reads", reads, document.GetAllocator());
    rapidjson::Value moves;
    moves.SetArray();
    for (auto move : hir->moves) {
        rapidjson::Value jsonMove;
        jsonMove.SetArray();
        jsonMove.PushBack(rapidjson::Value(move.first), document.GetAllocator());
        jsonMove.PushBack(rapidjson::Value(move.second), document.GetAllocator());
        moves.PushBack(jsonMove, document.GetAllocator());
    }
    jsonHIR.AddMember("moves", moves, document.GetAllocator());
    rapidjson::Value valueLocations;
    valueLocations.SetArray();
    for (auto loc : hir->valueLocations) {
        rapidjson::Value jsonLoc;
        jsonLoc.SetArray();
        jsonLoc.PushBack(rapidjson::Value(static_cast<uint64_t>(loc.first)), document.GetAllocator());
        jsonLoc.PushBack(rapidjson::Value(loc.second), document.GetAllocator());
        valueLocations.PushBack(jsonLoc, document.GetAllocator());
    }
    jsonHIR.AddMember("valueLocations", valueLocations, document.GetAllocator());
    switch(hir->opcode) {
    case hadron::hir::Opcode::kLoadArgument: {
        const auto loadArg = reinterpret_cast<const hadron::hir::LoadArgumentHIR*>(hir);
        jsonHIR.AddMember("opcode", "LoadArgument", document.GetAllocator());
        jsonHIR.AddMember("index", rapidjson::Value(loadArg->index), document.GetAllocator());
    } break;
    case hadron::hir::Opcode::kLoadArgumentType: {
        const auto loadArgType = reinterpret_cast<const hadron::hir::LoadArgumentTypeHIR*>(hir);
        jsonHIR.AddMember("opcode", "LoadArgumentType", document.GetAllocator());
        jsonHIR.AddMember("index", rapidjson::Value(loadArgType->index), document.GetAllocator());
    } break;
    case hadron::hir::Opcode::kConstant: {
        const auto constant = reinterpret_cast<const hadron::hir::ConstantHIR*>(hir);
        jsonHIR.AddMember("opcode", "Constant", document.GetAllocator());
        rapidjson::Value value;
        serializeSlot(constant->constant, value, document);
        jsonHIR.AddMember("constant", value, document.GetAllocator());
    } break;
    case hadron::hir::Opcode::kStoreReturn: {
        const auto storeReturn = reinterpret_cast<const hadron::hir::StoreReturnHIR*>(hir);
        jsonHIR.AddMember("opcode", "StoreReturn", document.GetAllocator());
        rapidjson::Value returnValue;
        returnValue.SetArray();
        rapidjson::Value value;
        serializeValue(storeReturn->returnValue.first, value, document);
        returnValue.PushBack(value, document.GetAllocator());
        rapidjson::Value type;
        serializeValue(storeReturn->returnValue.second, type, document);
        returnValue.PushBack(type, document.GetAllocator());
        jsonHIR.AddMember("returnValue", returnValue, document.GetAllocator());
    } break;

    case hadron::hir::Opcode::kLoadInstanceVariable:
    case hadron::hir::Opcode::kLoadInstanceVariableType:
    case hadron::hir::Opcode::kLoadClassVariable:
    case hadron::hir::Opcode::kLoadClassVariableType:
    case hadron::hir::Opcode::kStoreInstanceVariable:
    case hadron::hir::Opcode::kStoreClassVariable:
        assert(false); // TODO
        break;

    case hadron::hir::Opcode::kPhi: {
        const auto phi = reinterpret_cast<const hadron::hir::PhiHIR*>(hir);
        jsonHIR.AddMember("opcode", "Phi", document.GetAllocator());
        rapidjson::Value inputs;
        inputs.SetArray();
        for (auto input : phi->inputs) {
            rapidjson::Value value;
            serializeValue(input, value, document);
            inputs.PushBack(value, document.GetAllocator());
        }
        jsonHIR.AddMember("inputs", inputs, document.GetAllocator());
    } break;
    case hadron::hir::Opcode::kBranch: {
        const auto branch = reinterpret_cast<const hadron::hir::BranchHIR*>(hir);
        jsonHIR.AddMember("opcode", "Branch", document.GetAllocator());
        jsonHIR.AddMember("blockNumber", rapidjson::Value(branch->blockNumber), document.GetAllocator());
    } break;
    case hadron::hir::Opcode::kBranchIfZero: {
        const auto branchIfZero = reinterpret_cast<const hadron::hir::BranchIfZeroHIR*>(hir);
        jsonHIR.AddMember("opcode", "BranchIfZero", document.GetAllocator());
        jsonHIR.AddMember("blockNumber", rapidjson::Value(branchIfZero->blockNumber), document.GetAllocator());
        rapidjson::Value condition;
        condition.SetArray();
        rapidjson::Value value;
        serializeValue(branchIfZero->condition.first, value, document);
        condition.PushBack(value, document.GetAllocator());
        rapidjson::Value type;
        serializeValue(branchIfZero->condition.second, type, document);
        condition.PushBack(type, document.GetAllocator());
        jsonHIR.AddMember("condition", condition, document.GetAllocator());
    } break;
    case hadron::hir::Opcode::kLabel: {
        const auto label = reinterpret_cast<const hadron::hir::LabelHIR*>(hir);
        jsonHIR.AddMember("opcode", "Label", document.GetAllocator());
        jsonHIR.AddMember("blockNumber", rapidjson::Value(label->blockNumber), document.GetAllocator());
        rapidjson::Value predecessors;
        predecessors.SetArray();
        for (auto pred : label->predecessors) {
            predecessors.PushBack(rapidjson::Value(pred), document.GetAllocator());
        }
        jsonHIR.AddMember("predecessors", predecessors, document.GetAllocator());
        rapidjson::Value successors;
        successors.SetArray();
        for (auto succ : label->successors) {
            successors.PushBack(rapidjson::Value(succ), document.GetAllocator());
        }
        jsonHIR.AddMember("successors", successors, document.GetAllocator());
        rapidjson::Value phis;
        phis.SetArray();
        for (const auto& phi : label->phis) {
            rapidjson::Value jsonPhi;
            serializeHIR(phi.get(), jsonPhi, document);
            phis.PushBack(jsonPhi, document.GetAllocator());
        }
        jsonHIR.AddMember("phis", phis, document.GetAllocator());
    } break;
    case hadron::hir::Opcode::kDispatchSetupStack:
        jsonHIR.AddMember("opcode", "DispatchSetupStack", document.GetAllocator());
        break;
    case hadron::hir::Opcode::kDispatchStoreArg:
        jsonHIR.AddMember("opcode", "DispatchStoreArg", document.GetAllocator());
        break;
    case hadron::hir::Opcode::kDispatchStoreKeyArg:
        jsonHIR.AddMember("opcode", "DispatchStoreKeyArg", document.GetAllocator());
        break;
    case hadron::hir::Opcode::kDispatchCall:
        jsonHIR.AddMember("opcode", "DispatchCall", document.GetAllocator());
        break;
    case hadron::hir::Opcode::kDispatchLoadReturn:
        jsonHIR.AddMember("opcode", "DispatchLoadReturn", document.GetAllocator());
        break;
    case hadron::hir::Opcode::kDispatchLoadReturnType:
        jsonHIR.AddMember("opcode", "DispatchLoadReturnType", document.GetAllocator());
        break;
    case hadron::hir::Opcode::kDispatchCleanup:
        jsonHIR.AddMember("opcode", "DispatchCleanup", document.GetAllocator());
        break;
    }
}

void JSONTransport::JSONTransportImpl::serializeValue(hadron::Value value, rapidjson::Value& jsonValue,
        rapidjson::Document& document) {
    jsonValue.SetObject();
    jsonValue.AddMember("number", rapidjson::Value(static_cast<uint64_t>(value.number)), document.GetAllocator());
    rapidjson::Value typeFlags;
    typeFlags.SetArray();
    if (value.typeFlags == hadron::Type::kAny) {
        typeFlags.PushBack(rapidjson::Value("any"), document.GetAllocator());
    } else {
        if (value.typeFlags & hadron::Type::kNil) {
            typeFlags.PushBack(rapidjson::Value("nil"), document.GetAllocator());
        }
        if (value.typeFlags & hadron::Type::kInteger) {
            typeFlags.PushBack(rapidjson::Value("integer"), document.GetAllocator());
        }
        if (value.typeFlags & hadron::Type::kFloat) {
            typeFlags.PushBack(rapidjson::Value("float"), document.GetAllocator());
        }
        if (value.typeFlags & hadron::Type::kBoolean) {
            typeFlags.PushBack(rapidjson::Value("boolean"), document.GetAllocator());
        }
        if (value.typeFlags & hadron::Type::kString) {
            typeFlags.PushBack(rapidjson::Value("string"), document.GetAllocator());
        }
        if (value.typeFlags & hadron::Type::kSymbol) {
            typeFlags.PushBack(rapidjson::Value("symbol"), document.GetAllocator());
        }
        if (value.typeFlags & hadron::Type::kClass) {
            typeFlags.PushBack(rapidjson::Value("class"), document.GetAllocator());
        }
        if (value.typeFlags & hadron::Type::kObject) {
            typeFlags.PushBack(rapidjson::Value("object"), document.GetAllocator());
        }
        if (value.typeFlags & hadron::Type::kArray) {
            typeFlags.PushBack(rapidjson::Value("array"), document.GetAllocator());
        }
        if (value.typeFlags & hadron::Type::kType) {
            typeFlags.PushBack(rapidjson::Value("type"), document.GetAllocator());
        }
    }
    jsonValue.AddMember("typeFlags", typeFlags, document.GetAllocator());
}

void JSONTransport::JSONTransportImpl::serializeLifetimeIntervals(
        const std::vector<std::vector<hadron::LtIRef>>&lifetimeIntervals,
        rapidjson::Value& jsonIntervals, rapidjson::Document& document) {
    jsonIntervals.SetArray();
    for (const auto& value : lifetimeIntervals) {
        rapidjson::Value valueIntervals;
        valueIntervals.SetArray();
        for (const auto& lifetimeInterval : value) {
            rapidjson::Value interval;
            interval.SetObject();
            rapidjson::Value ranges;
            ranges.SetArray();
            for (const auto& range : lifetimeInterval->ranges) {
                rapidjson::Value jsonRange;
                jsonRange.SetObject();
                jsonRange.AddMember("from", rapidjson::Value(static_cast<uint64_t>(range.from)),
                        document.GetAllocator());
                jsonRange.AddMember("to", rapidjson::Value(static_cast<uint64_t>(range.to)), document.GetAllocator());
                ranges.PushBack(jsonRange, document.GetAllocator());
            }
            interval.AddMember("ranges", ranges, document.GetAllocator());

            rapidjson::Value usages;
            usages.SetArray();
            for (auto usage : lifetimeInterval->usages) {
                usages.PushBack(rapidjson::Value(static_cast<uint64_t>(usage)), document.GetAllocator());
            }
            interval.AddMember("usages", usages, document.GetAllocator());

            interval.AddMember("valueNumber", static_cast<uint64_t>(lifetimeInterval->valueNumber),
                    document.GetAllocator());
            interval.AddMember("registerNumber", static_cast<uint64_t>(lifetimeInterval->registerNumber),
                    document.GetAllocator());
            interval.AddMember("isSplit", rapidjson::Value(lifetimeInterval->isSplit), document.GetAllocator());
            interval.AddMember("isSpill", rapidjson::Value(lifetimeInterval->isSpill), document.GetAllocator());
            interval.AddMember("spillSlot", rapidjson::Value(static_cast<uint64_t>(lifetimeInterval->spillSlot)),
                    document.GetAllocator());
            valueIntervals.PushBack(interval, document.GetAllocator());
        }
        jsonIntervals.PushBack(valueIntervals, document.GetAllocator());
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

void JSONTransport::sendErrorResponse(std::optional<lsp::ID> id, ErrorCode errorCode, std::string errorMessage) {
    m_impl->sendErrorResponse(id, errorCode, errorMessage);
}

void JSONTransport::sendInitializeResult(std::optional<lsp::ID> id) {
    m_impl->sendInitializeResult(id);
}

void JSONTransport::sendSemanticTokens(const std::vector<hadron::Token>& tokens) {
    m_impl->sendSemanticTokens(tokens);
}

void JSONTransport::sendCompilationDiagnostics(lsp::ID id, const std::vector<CompilationUnit>& compilationUnits) {
    m_impl->sendCompilationDiagnostics(id, compilationUnits);
}

} // namespace server