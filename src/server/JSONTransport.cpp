#include "server/JSONTransport.hpp"

#include "hadron/AST.hpp"
#include "hadron/Block.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/Frame.hpp"
#include "hadron/hir/BranchHIR.hpp"
#include "hadron/hir/BranchIfTrueHIR.hpp"
#include "hadron/hir/ConstantHIR.hpp"
#include "hadron/hir/HIR.hpp"
#include "hadron/hir/LoadArgumentHIR.hpp"
#include "hadron/hir/MessageHIR.hpp"
#include "hadron/hir/MethodReturnHIR.hpp"
#include "hadron/hir/StoreReturnHIR.hpp"
#include "hadron/internal/BuildInfo.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/LifetimeInterval.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/lir/BranchIfTrueLIR.hpp"
#include "hadron/lir/BranchLIR.hpp"
#include "hadron/lir/BranchToRegisterLIR.hpp"
#include "hadron/lir/LabelLIR.hpp"
#include "hadron/lir/LIR.hpp"
#include "hadron/lir/LoadConstantLIR.hpp"
#include "hadron/lir/LoadFromPointerLIR.hpp"
#include "hadron/lir/LoadFromStackLIR.hpp"
#include "hadron/lir/PhiLIR.hpp"
#include "hadron/lir/StoreToPointerLIR.hpp"
#include "hadron/lir/StoreToStackLIR.hpp"
#include "hadron/OpcodeIterator.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Scope.hpp"
#include "hadron/ThreadContext.hpp"
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
#include <string_view>
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
    void sendCompilationDiagnostics(hadron::ThreadContext* context, lsp::ID id,
            const std::vector<CompilationUnit>& compilationUnits);

private:
    size_t readHeaders();
    void sendMessage(rapidjson::Document& document);
    bool handleMethod(const std::string& methodName, std::optional<lsp::ID> id, rapidjson::Document& document);
    bool handleResponse(lsp::ID id, rapidjson::Document& document);
    void encodeId(std::optional<lsp::ID> id, rapidjson::Document& document);

    void handleInitialize(std::optional<lsp::ID> id, const rapidjson::GenericObject<false, rapidjson::Value>& params);
    void serializeParseNode(hadron::ThreadContext* context, const hadron::parse::Node* node,
            rapidjson::Document& document, std::vector<rapidjson::Pointer::Token>& path, int& serial);
    void serializeAST(hadron::ThreadContext* context, const hadron::ast::AST* ast, rapidjson::Document& document,
            std::vector<rapidjson::Pointer::Token>& path, int& serial);
    void serializeSlot(hadron::ThreadContext* context, hadron::Slot slot, rapidjson::Value& target,
            rapidjson::Document& document);
    void serializeSymbol(hadron::ThreadContext* context, hadron::library::Symbol symbol, rapidjson::Value& target,
            rapidjson::Document& document);
    void serializeFrame(hadron::ThreadContext* context, const hadron::Frame* frame, rapidjson::Value& jsonFrame,
            rapidjson::Document& document);
    void serializeScope(hadron::ThreadContext* context, const hadron::Scope* scope, int& scopeSerial,
            rapidjson::Value& jsonScope, rapidjson::Document& document);
    void serializeLinearFrame(hadron::ThreadContext* context, const hadron::LinearFrame* linearFrame,
            rapidjson::Value& jsonBlock, rapidjson::Document& document);
    void serializeJIT(const int8_t* byteCode, size_t byteCodeSize, rapidjson::Value& jsonJIT,
            rapidjson::Document& document);
    void serializeHIR(hadron::ThreadContext* context, const hadron::hir::HIR* hir, const hadron::Frame* frame,
            rapidjson::Value& jsonHIR, rapidjson::Document& document);
    void serializeValue(hadron::ThreadContext* context, hadron::hir::NVID valueId, const hadron::Frame* frame,
            rapidjson::Value& jsonValue, rapidjson::Document& document);
    void serializeTypeFlags(hadron::Type typeFlags, rapidjson::Value& jsonTypeFlags, rapidjson::Document& document);
    void serializeLifetimeIntervals(const std::vector<std::vector<hadron::LtIRef>>& lifetimeIntervals,
            rapidjson::Value& jsonIntervals, rapidjson::Document& document);
    void serializeLIR(hadron::ThreadContext* context, const hadron::lir::LIR* lir, rapidjson::Value& jsonLIR,
            rapidjson::Document& document);

    static inline rapidjson::Pointer::Token makeToken(std::string_view v) {
        return rapidjson::Pointer::Token{v.data(), static_cast<rapidjson::SizeType>(v.size()),
                rapidjson::kPointerInvalidIndex};
    }

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

void JSONTransport::JSONTransportImpl::sendCompilationDiagnostics(hadron::ThreadContext* context, lsp::ID id,
        const std::vector<CompilationUnit>& compilationUnits) {
    rapidjson::Document document;
    document.SetObject();
    document.AddMember("jsonrpc", rapidjson::Value("2.0"), document.GetAllocator());
    encodeId(id, document);

    rapidjson::Value jsonUnits;
    jsonUnits.SetArray();

    for (const auto& unit : compilationUnits) {
        rapidjson::Value jsonUnit;
        jsonUnit.SetObject();

        SPDLOG_TRACE("Serializing compilation unit {}", unit.name);
        rapidjson::Value nameString;
        nameString.SetString(unit.name.data(), document.GetAllocator());
        jsonUnit.AddMember("name", nameString, document.GetAllocator());

        // Parse tree and AST are added using recursion and so are added after the rest of the members are complete.

        SPDLOG_TRACE("Serializing root frame for {}", unit.name);
        rapidjson::Value rootFrame;
        serializeFrame(context, unit.frame.get(), rootFrame, document);
        jsonUnit.AddMember("rootFrame", rootFrame, document.GetAllocator());

        SPDLOG_TRACE("Serializing linearFrame for {}", unit.name);
        rapidjson::Value jsonBlock;
        serializeLinearFrame(context, unit.linearFrame.get(), jsonBlock, document);
        jsonUnit.AddMember("linearFrame", jsonBlock, document.GetAllocator());

        SPDLOG_TRACE("Serializing machine code for {}", unit.name);
        rapidjson::Value jsonJIT;
        serializeJIT(unit.byteCode.get(), unit.byteCodeSize, jsonJIT, document);
        jsonUnit.AddMember("machineCode", jsonJIT, document.GetAllocator());

        jsonUnits.PushBack(jsonUnit, document.GetAllocator());
    }

    rapidjson::Value result;
    result.SetObject();
    // Note that due to move semantics in AddMember() |jsonUnits| is invalid after this add.
    result.AddMember("compilationUnits", jsonUnits, document.GetAllocator());
    document.AddMember("result", result, document.GetAllocator());

    // rapidjson::Pointer seems to want to anchor on existing nodes, so we add the parse and syntax trees to existing
    // objects in the compilationUnits array after its already been added to the DOM.
    for (size_t i = 0; i < compilationUnits.size(); ++i) {
        const auto& unit = compilationUnits[i];
        auto index = fmt::format("{}", i);

        int serial = 0;
        std::vector<rapidjson::Pointer::Token> parsePath({
                makeToken("result"),
                makeToken("compilationUnits"),
                {index.data(), static_cast<rapidjson::SizeType>(index.size()), static_cast<rapidjson::SizeType>(i)},
                makeToken("parseTree")});
        serializeParseNode(context, unit.blockNode, document, parsePath, serial);
        // On exit, path should be as we started.
        assert(parsePath.size() == 4);

        serial = 0;
        std::vector<rapidjson::Pointer::Token> astPath({
                makeToken("result"),
                makeToken("compilationUnits"),
                {index.data(), static_cast<rapidjson::SizeType>(index.size()), static_cast<rapidjson::SizeType>(i)},
                makeToken("ast") });
        serializeAST(context, unit.blockAST.get(), document, astPath, serial);
        assert(astPath.size() == 4);
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

void JSONTransport::JSONTransportImpl::serializeParseNode(hadron::ThreadContext* context,
        const hadron::parse::Node* node, rapidjson::Document& document, std::vector<rapidjson::Pointer::Token>& path,
        int& serial) {
    rapidjson::Value& jsonNode = rapidjson::CreateValueByPointer(document,
            rapidjson::Pointer(path.data(), path.size()), document.GetAllocator());
    jsonNode.SetObject();
    if (!node) {
        return;
    }
    jsonNode.AddMember("tokenIndex", rapidjson::Value(static_cast<uint64_t>(node->tokenIndex)),
            document.GetAllocator());

    jsonNode.AddMember("serial", rapidjson::Value(serial), document.GetAllocator());
    ++serial;

    switch(node->nodeType) {
    case hadron::parse::NodeType::kEmpty:
        jsonNode.AddMember("nodeType", rapidjson::Value("Empty"), document.GetAllocator());
        break;

    case hadron::parse::NodeType::kVarDef: {
        const auto varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("VarDef"), document.GetAllocator());
        jsonNode.AddMember("hasReadAccessor", rapidjson::Value(varDef->hasReadAccessor), document.GetAllocator());
        jsonNode.AddMember("hasWriteAccessor", rapidjson::Value(varDef->hasWriteAccessor), document.GetAllocator());
        path.emplace_back(makeToken("initialValue"));
        serializeParseNode(context, varDef->initialValue.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kVarList: {
        const auto varList = reinterpret_cast<const hadron::parse::VarListNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("VarList"), document.GetAllocator());
        path.emplace_back(makeToken("definitions"));
        serializeParseNode(context, varList->definitions.get(), document, path, serial);
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
        path.emplace_back(makeToken("varList"));
        serializeParseNode(context, argList->varList.get(), document, path, serial);
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
        path.emplace_back(makeToken("body"));
        serializeParseNode(context, method->body.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kClassExt: {
        const auto classExt = reinterpret_cast<const hadron::parse::ClassExtNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("ClassExt"), document.GetAllocator());
        path.emplace_back(makeToken("methods"));
        serializeParseNode(context, classExt->methods.get(), document, path, serial);
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
        path.emplace_back(makeToken("variables"));
        serializeParseNode(context, classNode->variables.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(makeToken("methods"));
        serializeParseNode(context, classNode->methods.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kReturn: {
        const auto retNode = reinterpret_cast<const hadron::parse::ReturnNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("ReturnNode"), document.GetAllocator());
        path.emplace_back(makeToken("valueExpr"));
        serializeParseNode(context, retNode->valueExpr.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kList: {
        const auto list = reinterpret_cast<const hadron::parse::ListNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("List"), document.GetAllocator());
        path.emplace_back(makeToken("elements"));;
        serializeParseNode(context, list->elements.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kDictionary: {
        const auto dict = reinterpret_cast<const hadron::parse::DictionaryNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Dictionary"), document.GetAllocator());
        path.emplace_back(makeToken("elements"));;
        serializeParseNode(context, dict->elements.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kBlock: {
        const auto block = reinterpret_cast<const hadron::parse::BlockNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Block"), document.GetAllocator());
        path.emplace_back(makeToken("arguments"));
        serializeParseNode(context, block->arguments.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(makeToken("variables"));
        serializeParseNode(context, block->variables.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(makeToken("body"));
        serializeParseNode(context, block->body.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kLiteral: {
        const auto literal = reinterpret_cast<const hadron::parse::LiteralNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Literal"), document.GetAllocator());
        rapidjson::Value value;
        serializeSlot(context, literal->value, value, document);
        jsonNode.AddMember("value", value, document.GetAllocator());
        path.emplace_back(makeToken("blockLiteral"));
        serializeParseNode(context, literal->blockLiteral.get(), document, path, serial);
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
        path.emplace_back(makeToken("expr"));
        serializeParseNode(context, exprSeq->expr.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kAssign: {
        const auto assign = reinterpret_cast<const hadron::parse::AssignNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Assign"), document.GetAllocator());
        path.emplace_back(makeToken("name"));
        serializeParseNode(context, assign->name.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(makeToken("value"));
        serializeParseNode(context, assign->value.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kSetter: {
        const auto setter = reinterpret_cast<const hadron::parse::SetterNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Setter"), document.GetAllocator());
        path.emplace_back(makeToken("target"));
        serializeParseNode(context, setter->target.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(makeToken("value"));
        serializeParseNode(context, setter->value.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kKeyValue: {
        const auto keyValue = reinterpret_cast<const hadron::parse::KeyValueNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("KeyValue"), document.GetAllocator());
        path.emplace_back(makeToken("value"));
        serializeParseNode(context, keyValue->value.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kCall: {
        const auto call = reinterpret_cast<const hadron::parse::CallNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Call"), document.GetAllocator());
        path.emplace_back(makeToken("target"));
        serializeParseNode(context, call->target.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(makeToken("arguments"));
        serializeParseNode(context, call->arguments.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(makeToken("keywordArguments"));
        serializeParseNode(context, call->keywordArguments.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kBinopCall: {
        const auto binop = reinterpret_cast<const hadron::parse::BinopCallNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("BinopCall"), document.GetAllocator());
        path.emplace_back(makeToken("leftHand"));
        serializeParseNode(context, binop->leftHand.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(makeToken("rightHand"));
        serializeParseNode(context, binop->rightHand.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(makeToken("adverb"));
        serializeParseNode(context, binop->adverb.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::parse::NodeType::kPerformList: {
        SPDLOG_WARN("Not yet implemented: kPerformList");
    } break;

    case hadron::parse::NodeType::kNumericSeries: {
        SPDLOG_WARN("Not yet implemented: kNumericSeries");
    } break;

    case hadron::parse::NodeType::kCurryArgument: {
        SPDLOG_WARN("Not yet implemented: kCurryArgument");
    } break;

    case hadron::parse::NodeType::kArrayRead: {
        SPDLOG_WARN("Not yet implemented: kArrayRead");
    } break;

    case hadron::parse::NodeType::kArrayWrite: {
        SPDLOG_WARN("Not yet implemented: kArrayWrite");
    } break;

    case hadron::parse::NodeType::kCopySeries: {
        SPDLOG_WARN("Not yet implemented: kCopySeries");
    } break;

    case hadron::parse::NodeType::kNew: {
        SPDLOG_WARN("Not yet implemented: kNew");
    } break;

    case hadron::parse::NodeType::kSeries: {
        SPDLOG_WARN("Not yet implemented: kSeries");
    } break;

    case hadron::parse::NodeType::kSeriesIter: {
        SPDLOG_WARN("Not yet implemented: kSeriesIter");
    } break;

    case hadron::parse::NodeType::kLiteralList: {
        SPDLOG_WARN("Not yet implemented: kLiteralList");
    } break;

    case hadron::parse::NodeType::kLiteralDict: {
        SPDLOG_WARN("Not yet implemented: kLiteralDict");
    } break;

    case hadron::parse::NodeType::kMultiAssignVars: {
        SPDLOG_WARN("Not yet implemented: kMultiAssignVars");
    } break;

    case hadron::parse::NodeType::kMultiAssign: {
        SPDLOG_WARN("Not yet implemented: kMultiAssign");
    } break;

    case hadron::parse::NodeType::kIf: {
        const auto ifNode = reinterpret_cast<const hadron::parse::IfNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("If"), document.GetAllocator());
        path.emplace_back(makeToken("condition"));
        serializeParseNode(context, ifNode->condition.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(makeToken("trueBlock"));
        serializeParseNode(context, ifNode->trueBlock.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(makeToken("falseBlock"));
        serializeParseNode(context, ifNode->falseBlock.get(), document, path, serial);
        path.pop_back();
    } break;
    }

    path.emplace_back(makeToken("next"));
    serializeParseNode(context, node->next.get(), document, path, serial);
    path.pop_back();
}

void JSONTransport::JSONTransportImpl::serializeAST(hadron::ThreadContext* context, const hadron::ast::AST* ast,
        rapidjson::Document& document, std::vector<rapidjson::Pointer::Token>& path, int& serial) {
    rapidjson::Value& jsonNode = rapidjson::CreateValueByPointer(document, rapidjson::Pointer(path.data(), path.size()),
            document.GetAllocator());
    jsonNode.SetObject();
    jsonNode.AddMember("serial", rapidjson::Value(serial), document.GetAllocator());
    ++serial;

    switch(ast->astType) {
    case hadron::ast::ASTType::kAssign: {
        const auto assignAST = reinterpret_cast<const hadron::ast::AssignAST*>(ast);
        jsonNode.AddMember("astType", rapidjson::Value("Assign"), document.GetAllocator());
        path.emplace_back(makeToken("name"));
        serializeAST(context, assignAST->name.get(), document, path, serial);
        path.pop_back();
        path.emplace_back(makeToken("value"));
        serializeAST(context, assignAST->value.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::ast::ASTType::kBlock: {
        const auto blockAST = reinterpret_cast<const hadron::ast::BlockAST*>(ast);
        jsonNode.AddMember("astType", rapidjson::Value("Block"), document.GetAllocator());

        rapidjson::Value argumentNames;
        argumentNames.SetArray();
        for (int32_t i = 0; i < blockAST->argumentNames.size(); ++i) {
            rapidjson::Value argName;
            serializeSymbol(context, blockAST->argumentNames.at(i), argName, document);
            argumentNames.PushBack(argName, document.GetAllocator());
        }
        jsonNode.AddMember("argumentNames", argumentNames, document.GetAllocator());

        rapidjson::Value argumentDefaults;
        argumentDefaults.SetArray();
        for (int32_t i = 0; i < blockAST->argumentDefaults.size(); ++i) {
            rapidjson::Value argDefault;
            serializeSlot(context, blockAST->argumentDefaults.at(i), argDefault, document);
            argumentDefaults.PushBack(argDefault, document.GetAllocator());
        }
        jsonNode.AddMember("argumentDefaults", argumentDefaults, document.GetAllocator());

        jsonNode.AddMember("hasVarArg", rapidjson::Value(blockAST->hasVarArg), document.GetAllocator());

        path.emplace_back(makeToken("statements"));
        serializeAST(context, blockAST->statements.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::ast::ASTType::kConstant: {
        const auto constAST = reinterpret_cast<const hadron::ast::ConstantAST*>(ast);
        jsonNode.AddMember("astType", rapidjson::Value("Constant"), document.GetAllocator());

        rapidjson::Value value;
        serializeSlot(context, constAST->constant, value, document);
        jsonNode.AddMember("constant", value, document.GetAllocator());
    } break;

    case hadron::ast::ASTType::kDictionary: {
        SPDLOG_WARN("Not yet implemented: kDictionary");
    } break;

    case hadron::ast::ASTType::kEmpty: {
        jsonNode.AddMember("astType", rapidjson::Value("Empty"), document.GetAllocator());
    } break;

    case hadron::ast::ASTType::kIf: {
        const auto ifAST = reinterpret_cast<const hadron::ast::IfAST*>(ast);
        jsonNode.AddMember("astType", rapidjson::Value("If"), document.GetAllocator());

        path.emplace_back(makeToken("condition"));
        serializeAST(context, ifAST->condition.get(), document, path, serial);
        path.pop_back();

        path.emplace_back(makeToken("trueBlock"));
        serializeAST(context, ifAST->trueBlock.get(), document, path, serial);
        path.pop_back();

        path.emplace_back(makeToken("falseBlock"));
        serializeAST(context, ifAST->falseBlock.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::ast::ASTType::kList: {
        SPDLOG_WARN("Not yet implemented: kList");
    } break;

    case hadron::ast::ASTType::kMessage: {
        const auto messageAST = reinterpret_cast<const hadron::ast::MessageAST*>(ast);
        jsonNode.AddMember("astType", rapidjson::Value("Message"), document.GetAllocator());

        path.emplace_back(makeToken("target"));
        serializeAST(context, messageAST->target.get(), document, path, serial);
        path.pop_back();

        rapidjson::Value selector;
        serializeSymbol(context, messageAST->selector, selector, document);
        jsonNode.AddMember("selector", selector, document.GetAllocator());

        path.emplace_back(makeToken("arguments"));
        serializeAST(context, messageAST->arguments.get(), document, path, serial);
        path.pop_back();

        path.emplace_back(makeToken("keywordArguments"));
        serializeAST(context, messageAST->keywordArguments.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::ast::ASTType::kMethodReturn: {
        const auto returnAST = reinterpret_cast<const hadron::ast::MethodReturnAST*>(ast);
        jsonNode.AddMember("astType", rapidjson::Value("MethodReturn"), document.GetAllocator());

        path.emplace_back(makeToken("value"));
        serializeAST(context, returnAST->value.get(), document, path, serial);
        path.pop_back();
    } break;

    case hadron::ast::ASTType::kName: {
        const auto nameAST = reinterpret_cast<const hadron::ast::NameAST*>(ast);
        jsonNode.AddMember("astType", rapidjson::Value("Name"), document.GetAllocator());

        rapidjson::Value name;
        serializeSymbol(context, nameAST->name, name, document);
        jsonNode.AddMember("name", name, document.GetAllocator());
    } break;

    case hadron::ast::ASTType::kSequence: {
        const auto sequenceAST = reinterpret_cast<const hadron::ast::SequenceAST*>(ast);
        jsonNode.AddMember("astType", rapidjson::Value("Sequence"), document.GetAllocator());

        rapidjson::Value sequence;
        sequence.SetArray();
        sequence.Reserve(static_cast<rapidjson::SizeType>(sequenceAST->sequence.size()), document.GetAllocator());
        path.emplace_back(makeToken("sequence"));
        int i = 0;
        for (const auto& seq : sequenceAST->sequence) {
            auto index = fmt::format("{}", i);
            path.emplace_back(rapidjson::Pointer::Token{index.data(),
                    static_cast<rapidjson::SizeType>(index.size()), static_cast<rapidjson::SizeType>(i)});
            serializeAST(context, seq.get(), document, path, serial);
            path.pop_back();
            ++i;
        }
        path.pop_back();
    } break;
    }
}


void JSONTransport::JSONTransportImpl::serializeSlot(hadron::ThreadContext* context, hadron::Slot slot,
        rapidjson::Value& target, rapidjson::Document& document) {
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
    case hadron::Type::kSymbol: {
        target.AddMember("type", rapidjson::Value("symbol"), document.GetAllocator());
        rapidjson::Value value;
        serializeSymbol(context, hadron::library::Symbol::fromHash(context, slot.getHash()), value, document);
        target.AddMember("value", value, document.GetAllocator());
    } break;
    default:
        target.AddMember("type", rapidjson::Value("unknown"), document.GetAllocator());
        break;
    }
}

void JSONTransport::JSONTransportImpl::serializeSymbol(hadron::ThreadContext* context, hadron::library::Symbol symbol,
        rapidjson::Value& target, rapidjson::Document& document) {
    target.SetObject();
    target.AddMember("hash", rapidjson::Value(symbol.hash()), document.GetAllocator());
    rapidjson::Value stringValue;
    stringValue.SetString(symbol.view(context).data(), symbol.view(context).size());
    target.AddMember("string", stringValue, document.GetAllocator());
}

void JSONTransport::JSONTransportImpl::serializeFrame(hadron::ThreadContext* context, const hadron::Frame* frame,
        rapidjson::Value& jsonFrame, rapidjson::Document& document) {
    jsonFrame.SetObject();
    // TODO: argumentOrder
    rapidjson::Value rootScope;
    int scopeSerial = 1;
    serializeScope(context, frame->rootScope.get(), scopeSerial, rootScope, document);
    jsonFrame.AddMember("rootScope", rootScope, document.GetAllocator());

    jsonFrame.AddMember("numberOfBlocks", rapidjson::Value(frame->numberOfBlocks), document.GetAllocator());
}

void JSONTransport::JSONTransportImpl::serializeScope(hadron::ThreadContext* context, const hadron::Scope* scope,
        int& scopeSerial, rapidjson::Value& jsonScope, rapidjson::Document& document) {
    jsonScope.SetObject();
    int serial = scopeSerial;
    SPDLOG_TRACE("serializing Scope {}", serial);
    ++scopeSerial;
    jsonScope.AddMember("scopeSerial", rapidjson::Value(serial), document.GetAllocator());

    rapidjson::Value blocks;
    blocks.SetArray();

    for (const auto& block : scope->blocks) {
        SPDLOG_TRACE("serializing Block {}", block->id);

        rapidjson::Value jsonBlock;
        jsonBlock.SetObject();
        jsonBlock.AddMember("id", rapidjson::Value(block->id), document.GetAllocator());

        rapidjson::Value predecessors;
        predecessors.SetArray();
        for (const auto pred : block->predecessors) {
            predecessors.PushBack(rapidjson::Value(pred->id), document.GetAllocator());
        }
        jsonBlock.AddMember("predecessors", predecessors, document.GetAllocator());

        rapidjson::Value successors;
        successors.SetArray();
        for (const auto succ : block->successors) {
            successors.PushBack(rapidjson::Value(succ->id), document.GetAllocator());
        }
        jsonBlock.AddMember("successors", successors, document.GetAllocator());

        rapidjson::Value phis;
        phis.SetArray();
        for (const auto& phi : block->phis) {
            rapidjson::Value jsonPhi;
            serializeHIR(context, phi.get(), scope->frame, jsonPhi, document);
            phis.PushBack(jsonPhi, document.GetAllocator());
        }
        jsonBlock.AddMember("phis", phis, document.GetAllocator());

        rapidjson::Value statements;
        statements.SetArray();
        for (const auto& hir : block->statements) {
            rapidjson::Value jsonHIR;
            serializeHIR(context, hir.get(), scope->frame, jsonHIR, document);
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
        serializeScope(context, subScope.get(), scopeSerial, jsonSubScope, document);
        subScopes.PushBack(jsonSubScope, document.GetAllocator());
    }
    jsonScope.AddMember("subScopes", subScopes, document.GetAllocator());
}

void JSONTransport::JSONTransportImpl::serializeLinearFrame(hadron::ThreadContext* context,
        const hadron::LinearFrame* linearFrame, rapidjson::Value& jsonBlock, rapidjson::Document& document) {
    jsonBlock.SetObject();
    rapidjson::Value instructions;
    instructions.SetArray();
    for (const auto& lir : linearFrame->instructions) {
        rapidjson::Value jsonLIR;
        serializeLIR(context, lir.get(), jsonLIR, document);
        instructions.PushBack(jsonLIR, document.GetAllocator());
    }
    jsonBlock.AddMember("instructions", instructions, document.GetAllocator());

    rapidjson::Value blockOrder;
    blockOrder.SetArray();
    for (auto number : linearFrame->blockOrder) {
        blockOrder.PushBack(rapidjson::Value(number), document.GetAllocator());
    }
    jsonBlock.AddMember("blockOrder", blockOrder, document.GetAllocator());

    rapidjson::Value blockRanges;
    blockRanges.SetArray();
    for (auto range : linearFrame->blockRanges) {
        rapidjson::Value jsonRange;
        jsonRange.SetArray();
        jsonRange.PushBack(rapidjson::Value(static_cast<uint64_t>(range.first)), document.GetAllocator());
        jsonRange.PushBack(rapidjson::Value(static_cast<uint64_t>(range.second)), document.GetAllocator());
        blockRanges.PushBack(jsonRange, document.GetAllocator());
    }
    jsonBlock.AddMember("blockRanges", blockRanges, document.GetAllocator());

    rapidjson::Value valueLifetimes;
    serializeLifetimeIntervals(linearFrame->valueLifetimes, valueLifetimes, document);
    jsonBlock.AddMember("valueLifetimes", valueLifetimes, document.GetAllocator());

    jsonBlock.AddMember("numberOfSpillSlots", rapidjson::Value(static_cast<uint64_t>(linearFrame->numberOfSpillSlots)),
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
        case hadron::Opcode::kMoviU: {
            opcode.PushBack("movi_u", document.GetAllocator());
            hadron::JIT::Reg target;
            hadron::UWord value;
            it.movi_u(target, value);
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

void JSONTransport::JSONTransportImpl::serializeHIR(hadron::ThreadContext* context, const hadron::hir::HIR* hir,
        const hadron::Frame* frame, rapidjson::Value& jsonHIR, rapidjson::Document& document) {
    jsonHIR.SetObject();

    if (hir->value.id != hadron::hir::kInvalidNVID) {
        rapidjson::Value value;
        serializeValue(context, hir->value.id, frame, value, document);
        jsonHIR.AddMember("value", value, document.GetAllocator());
    }

    rapidjson::Value reads;
    reads.SetArray();
    for (auto read : hir->reads) {
        rapidjson::Value jsonRead;
        serializeValue(context, read, frame, jsonRead, document);
        reads.PushBack(jsonRead, document.GetAllocator());
    }
    jsonHIR.AddMember("reads", reads, document.GetAllocator());

    switch(hir->opcode) {
    case hadron::hir::Opcode::kBranch: {
        const auto branch = reinterpret_cast<const hadron::hir::BranchHIR*>(hir);
        jsonHIR.AddMember("opcode", "Branch", document.GetAllocator());
        jsonHIR.AddMember("blockId", rapidjson::Value(branch->blockId), document.GetAllocator());
    } break;

    case hadron::hir::Opcode::kBranchIfTrue: {
        const auto branchIfTrue = reinterpret_cast<const hadron::hir::BranchIfTrueHIR*>(hir);
        jsonHIR.AddMember("opcode", "BranchIfTrue", document.GetAllocator());
        jsonHIR.AddMember("blockId", rapidjson::Value(branchIfTrue->blockId), document.GetAllocator());

        rapidjson::Value condition;
        serializeValue(context, branchIfTrue->condition, frame, condition, document);
        jsonHIR.AddMember("condition", condition, document.GetAllocator());
    } break;

    case hadron::hir::Opcode::kConstant: {
        const auto constant = reinterpret_cast<const hadron::hir::ConstantHIR*>(hir);
        jsonHIR.AddMember("opcode", "Constant", document.GetAllocator());

        rapidjson::Value value;
        serializeSlot(context, constant->constant, value, document);
        jsonHIR.AddMember("constant", value, document.GetAllocator());
    } break;

    case hadron::hir::Opcode::kLoadArgument: {
        const auto loadArg = reinterpret_cast<const hadron::hir::LoadArgumentHIR*>(hir);
        jsonHIR.AddMember("opcode", "LoadArgument", document.GetAllocator());
        jsonHIR.AddMember("index", rapidjson::Value(loadArg->index), document.GetAllocator());
    } break;

    case hadron::hir::Opcode::kLoadClassVariable: {
        SPDLOG_WARN("Not yet implemented: LoadClassVariable");
    } break;

    case hadron::hir::Opcode::kLoadInstanceVariable: {
        SPDLOG_WARN("Not yet implemented: LoadInstanceVariable");
    } break;

    case hadron::hir::Opcode::kMessage: {
        const auto message = reinterpret_cast<const hadron::hir::MessageHIR*>(hir);
        jsonHIR.AddMember("opcode", "Message", document.GetAllocator());

        rapidjson::Value selector;
        serializeSymbol(context, message->selector, selector, document);
        jsonHIR.AddMember("selector", selector, document.GetAllocator());

        rapidjson::Value arguments;
        arguments.SetArray();
        for (auto argId : message->arguments) {
            rapidjson::Value arg;
            serializeValue(context, argId, frame, arg, document);
            arguments.PushBack(arg, document.GetAllocator());
        }
        jsonHIR.AddMember("arguments", arguments, document.GetAllocator());

        rapidjson::Value keywordArguments;
        keywordArguments.SetArray();
        for (auto argId : message->keywordArguments) {
            rapidjson::Value arg;
            serializeValue(context, argId, frame, arg, document);
            keywordArguments.PushBack(arg, document.GetAllocator());
        }
        jsonHIR.AddMember("keywordArguments", keywordArguments, document.GetAllocator());
    } break;

    case hadron::hir::Opcode::kMethodReturn: {
        jsonHIR.AddMember("opcode", "MethodReturn", document.GetAllocator());
    } break;

    case hadron::hir::Opcode::kPhi: {
        const auto phi = reinterpret_cast<const hadron::hir::PhiHIR*>(hir);
        jsonHIR.AddMember("opcode", "Phi", document.GetAllocator());

        rapidjson::Value inputs;
        inputs.SetArray();
        for (auto input : phi->inputs) {
            rapidjson::Value value;
            serializeValue(context, input, frame, value, document);
            inputs.PushBack(value, document.GetAllocator());
        }
        jsonHIR.AddMember("inputs", inputs, document.GetAllocator());
    } break;

    case hadron::hir::Opcode::kStoreClassVariable: {
        SPDLOG_WARN("Not yet implemented: StoreClassVariable");
    } break;

    case hadron::hir::Opcode::kStoreInstanceVariable: {
        SPDLOG_WARN("Not yet implemented: StoreInstanceVariable");
    } break;

    case hadron::hir::Opcode::kStoreReturn: {
       const auto ret = reinterpret_cast<const hadron::hir::StoreReturnHIR*>(hir);
       jsonHIR.AddMember("opcode", "StoreReturn", document.GetAllocator());

        rapidjson::Value returnValue;
        serializeValue(context, ret->returnValue, frame, returnValue, document);
        jsonHIR.AddMember("returnValue", returnValue, document.GetAllocator());
    } break;
    }
}

void JSONTransport::JSONTransportImpl::serializeValue(hadron::ThreadContext* context, hadron::hir::NVID valueId,
        const hadron::Frame* frame, rapidjson::Value& jsonValue, rapidjson::Document& document) {
    assert(valueId != hadron::hir::kInvalidNVID);

    jsonValue.SetObject();
    jsonValue.AddMember("id", rapidjson::Value(valueId), document.GetAllocator());

    rapidjson::Value jsonTypeFlags;
    serializeTypeFlags(frame->values[valueId]->value.typeFlags, jsonTypeFlags, document);
    jsonValue.AddMember("typeFlags", jsonTypeFlags, document.GetAllocator());

    if (!frame->values[valueId]->value.name.isNil()) {
        rapidjson::Value name;
        auto view = frame->values[valueId]->value.name.view(context);
        name.SetString(view.data(), view.size(), document.GetAllocator());
        jsonValue.AddMember("name", name, document.GetAllocator());
    }
}

void JSONTransport::JSONTransportImpl::serializeTypeFlags(hadron::Type typeFlags, rapidjson::Value& jsonTypeFlags,
        rapidjson::Document& document) {
    jsonTypeFlags.SetArray();
    if (typeFlags == hadron::Type::kAny) {
        jsonTypeFlags.PushBack(rapidjson::Value("any"), document.GetAllocator());
    } else {
        if (typeFlags & hadron::Type::kNil) {
            jsonTypeFlags.PushBack(rapidjson::Value("nil"), document.GetAllocator());
        }
        if (typeFlags & hadron::Type::kInteger) {
            jsonTypeFlags.PushBack(rapidjson::Value("int"), document.GetAllocator());
        }
        if (typeFlags & hadron::Type::kFloat) {
            jsonTypeFlags.PushBack(rapidjson::Value("float"), document.GetAllocator());
        }
        if (typeFlags & hadron::Type::kBoolean) {
            jsonTypeFlags.PushBack(rapidjson::Value("bool"), document.GetAllocator());
        }
        if (typeFlags & hadron::Type::kString) {
            jsonTypeFlags.PushBack(rapidjson::Value("string"), document.GetAllocator());
        }
        if (typeFlags & hadron::Type::kSymbol) {
            jsonTypeFlags.PushBack(rapidjson::Value("symbol"), document.GetAllocator());
        }
        if (typeFlags & hadron::Type::kClass) {
            jsonTypeFlags.PushBack(rapidjson::Value("class"), document.GetAllocator());
        }
        if (typeFlags & hadron::Type::kObject) {
            jsonTypeFlags.PushBack(rapidjson::Value("object"), document.GetAllocator());
        }
        if (typeFlags & hadron::Type::kArray) {
            jsonTypeFlags.PushBack(rapidjson::Value("array"), document.GetAllocator());
        }
        if (typeFlags & hadron::Type::kType) {
            jsonTypeFlags.PushBack(rapidjson::Value("type"), document.GetAllocator());
        }
    }
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

void JSONTransport::JSONTransportImpl::serializeLIR(hadron::ThreadContext* context, const hadron::lir::LIR* lir,
        rapidjson::Value& jsonLIR, rapidjson::Document& document) {
    jsonLIR.SetObject();
    if (lir->value != hadron::lir::kInvalidVReg) {
        jsonLIR.AddMember("value", rapidjson::Value(lir->value), document.GetAllocator());
        rapidjson::Value typeFlags;
        serializeTypeFlags(lir->typeFlags, typeFlags, document);
        jsonLIR.AddMember("typeFlags", typeFlags, document.GetAllocator());
    }

    rapidjson::Value moves;
    moves.SetArray();
    for (auto move : lir->moves) {
        rapidjson::Value jsonMove;
        jsonMove.SetArray();
        jsonMove.PushBack(rapidjson::Value(move.first), document.GetAllocator());
        jsonMove.PushBack(rapidjson::Value(move.second), document.GetAllocator());
        moves.PushBack(jsonMove, document.GetAllocator());
    }
    jsonLIR.AddMember("moves", moves, document.GetAllocator());
 
    rapidjson::Value valueLocations;
    valueLocations.SetArray();
    for (auto loc : lir->locations) {
        rapidjson::Value jsonLoc;
        jsonLoc.SetArray();
        jsonLoc.PushBack(rapidjson::Value(static_cast<uint64_t>(loc.first)), document.GetAllocator());
        jsonLoc.PushBack(rapidjson::Value(loc.second), document.GetAllocator());
        valueLocations.PushBack(jsonLoc, document.GetAllocator());
    }
    jsonLIR.AddMember("locations", valueLocations, document.GetAllocator());

    switch(lir->opcode) {
    case hadron::lir::Opcode::kBranch: {
        const auto branch = reinterpret_cast<const hadron::lir::BranchLIR*>(lir);
        jsonLIR.AddMember("opcode", "Branch", document.GetAllocator());
        jsonLIR.AddMember("labelId", rapidjson::Value(branch->labelId), document.GetAllocator());
    } break;

    case hadron::lir::Opcode::kBranchIfTrue: {
        const auto branchIfTrue = reinterpret_cast<const hadron::lir::BranchIfTrueLIR*>(lir);
        jsonLIR.AddMember("opcode", "BranchIfTrue", document.GetAllocator());
        jsonLIR.AddMember("condition", rapidjson::Value(branchIfTrue->condition), document.GetAllocator());
        jsonLIR.AddMember("labelId", rapidjson::Value(branchIfTrue->labelId), document.GetAllocator());
    } break;

    case hadron::lir::Opcode::kBranchToRegister: {
        const auto branchToRegister = reinterpret_cast<const hadron::lir::BranchToRegisterLIR*>(lir);
        jsonLIR.AddMember("opcode", "BranchToRegister", document.GetAllocator());
        jsonLIR.AddMember("address", rapidjson::Value(branchToRegister->address), document.GetAllocator());
    } break;

    case hadron::lir::Opcode::kLabel: {
        const auto label = reinterpret_cast<const hadron::lir::LabelLIR*>(lir);
        jsonLIR.AddMember("opcode", "Label", document.GetAllocator());
        jsonLIR.AddMember("id", rapidjson::Value(label->id), document.GetAllocator());

        rapidjson::Value predecessors;
        predecessors.SetArray();
        for (auto pred : label->predecessors) {
            predecessors.PushBack(rapidjson::Value(pred), document.GetAllocator());
        }
        jsonLIR.AddMember("predecessors", predecessors, document.GetAllocator());

        rapidjson::Value successors;
        successors.SetArray();
        for (auto succ : label->successors) {
            successors.PushBack(rapidjson::Value(succ), document.GetAllocator());
        }
        jsonLIR.AddMember("successors", successors, document.GetAllocator());

        rapidjson::Value phis;
        phis.SetArray();
        for (const auto& phi : label->phis) {
            rapidjson::Value jsonPhi;
            serializeLIR(context, phi.get(), jsonPhi, document);
            phis.PushBack(jsonPhi, document.GetAllocator());
        }
        jsonLIR.AddMember("phis", phis, document.GetAllocator());
    } break;

    case hadron::lir::kLoadConstant: {
        const auto loadConstant = reinterpret_cast<const hadron::lir::LoadConstantLIR*>(lir);
        jsonLIR.AddMember("opcode", "LoadConstant", document.GetAllocator());

        rapidjson::Value constant;
        serializeSlot(context, loadConstant->constant, constant, document);
        jsonLIR.AddMember("constant", constant, document.GetAllocator());
    } break;

    case hadron::lir::kLoadFromPointer: {
        const auto loadPointer = reinterpret_cast<const hadron::lir::LoadFromPointerLIR*>(lir);
        jsonLIR.AddMember("opcode", "LoadFromPointer", document.GetAllocator());
        jsonLIR.AddMember("pointer", rapidjson::Value(loadPointer->pointer), document.GetAllocator());
        jsonLIR.AddMember("offset", rapidjson::Value(loadPointer->offset), document.GetAllocator());
    } break;

    case hadron::lir::kLoadFromStack: {
        const auto loadStack = reinterpret_cast<const hadron::lir::LoadFromStackLIR*>(lir);
        jsonLIR.AddMember("opcode", "LoadFromStack", document.GetAllocator());
        jsonLIR.AddMember("useFramePointer", rapidjson::Value(loadStack->useFramePointer), document.GetAllocator());
        jsonLIR.AddMember("offset", rapidjson::Value(loadStack->offset), document.GetAllocator());
    } break;

    case hadron::lir::kPhi: {
        const auto phi = reinterpret_cast<const hadron::lir::PhiLIR*>(lir);
        jsonLIR.AddMember("opcode", "Phi", document.GetAllocator());

        rapidjson::Value inputs;
        inputs.SetArray();
        for (auto vReg : phi->inputs) {
            inputs.PushBack(rapidjson::Value(vReg), document.GetAllocator());
        }
        jsonLIR.AddMember("inputs", inputs, document.GetAllocator());
    } break;

    case hadron::lir::kStoreToPointer: {
        const auto storePointer = reinterpret_cast<const hadron::lir::StoreToPointerLIR*>(lir);
        jsonLIR.AddMember("opcode", "StoreToPointer", document.GetAllocator());
        jsonLIR.AddMember("pointer", rapidjson::Value(storePointer->pointer), document.GetAllocator());
        jsonLIR.AddMember("toStore", rapidjson::Value(storePointer->toStore), document.GetAllocator());
        jsonLIR.AddMember("offset", rapidjson::Value(storePointer->offset), document.GetAllocator());
    } break;

    case hadron::lir::kStoreToStack: {
        const auto storeStack = reinterpret_cast<const hadron::lir::StoreToStackLIR*>(lir);
        jsonLIR.AddMember("opcode", "StoreToStack", document.GetAllocator());
        jsonLIR.AddMember("toStore", rapidjson::Value(storeStack->toStore), document.GetAllocator());
        jsonLIR.AddMember("useFramePointer", rapidjson::Value(storeStack->useFramePointer), document.GetAllocator());
        jsonLIR.AddMember("offset", rapidjson::Value(storeStack->offset), document.GetAllocator());
    } break;
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

void JSONTransport::sendCompilationDiagnostics(hadron::ThreadContext* context, lsp::ID id,
        const std::vector<CompilationUnit>& compilationUnits) {
    m_impl->sendCompilationDiagnostics(context, id, compilationUnits);
}

} // namespace server