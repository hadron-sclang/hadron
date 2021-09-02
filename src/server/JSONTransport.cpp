#include "server/JSONTransport.hpp"

#include "hadron/Parser.hpp"
#include "internal/BuildInfo.hpp"
#include "server/HadronServer.hpp"
#include "server/LSPMethods.hpp"

#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
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
    void sendParseTree(lsp::ID id, const hadron::parse::Node* rootNode);

private:
    size_t readHeaders();
    void sendMessage(rapidjson::Document& document);
    bool handleMethod(const std::string& methodName, std::optional<lsp::ID> id, rapidjson::Document& document);
    bool handleResponse(lsp::ID id, rapidjson::Document& document);
    void encodeId(std::optional<lsp::ID> id, rapidjson::Document& document);

    void handleInitialize(std::optional<lsp::ID> id, const rapidjson::GenericObject<false, rapidjson::Value>& params);
    void serializeParseNode(const hadron::parse::Node* node, rapidjson::Document& document,
            std::vector<rapidjson::Pointer::Token>& path);
    void serializeSlot(hadron::Slot slot, rapidjson::Value& target, rapidjson::Document& document);

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
    serverInfo.AddMember("version", rapidjson::Value(hadron::kHadronVersion, document.GetAllocator()),
            document.GetAllocator());
    result.AddMember("serverInfo", serverInfo, document.GetAllocator());
    document.AddMember("result", result, document.GetAllocator());
    sendMessage(document);
}

void JSONTransport::JSONTransportImpl::sendParseTree(lsp::ID id, const hadron::parse::Node* rootNode) {
    rapidjson::Document document;
    document.SetObject();
    document.AddMember("jsonrpc", rapidjson::Value("2.0"), document.GetAllocator());
    encodeId(id, document);
    std::vector<rapidjson::Pointer::Token> path({{"parseTree", sizeof("parseTree") - 1,
            rapidjson::kPointerInvalidIndex}});
    serializeParseNode(rootNode, document, path);
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

    case server::lsp::Method::kHadronParseTree: {
        if (!params || !params->HasMember("uri") || !(*params)["uri"].IsString()) {
            sendErrorResponse(id, ErrorCode::kInvalidParams, "Absent or malformed params key in 'hadron/parseTree' "
                    "method.");
            return false;
        }
        m_server->hadronParseTree(*id, (*params)["uri"].GetString());
    } break;

    case server::lsp::Method::kHadronBlockFlow:
        break;
    case server::lsp::Method::kHadronLinearBlock:
        break;
    case server::lsp::Method::kHadronMachineCode:
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

void JSONTransport::JSONTransportImpl::serializeParseNode(const hadron::parse::Node* node,
        rapidjson::Document& document, std::vector<rapidjson::Pointer::Token>& path) {
    rapidjson::Value& jsonNode = rapidjson::CreateValueByPointer(document,
            rapidjson::Pointer(path.data(), path.size()), document.GetAllocator());
    if (!node) {
        return;
    }
    jsonNode.SetObject();
    jsonNode.AddMember("tokenIndex", rapidjson::Value(static_cast<uint64_t>(node->tokenIndex)),
            document.GetAllocator());
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
        serializeParseNode(varDef->initialValue.get(), document, path);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kVarList: {
        const auto varList = reinterpret_cast<const hadron::parse::VarListNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("VarList"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"definitions", sizeof("definitions") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(varList->definitions.get(), document, path);
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
        serializeParseNode(argList->varList.get(), document, path);
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
        serializeParseNode(method->body.get(), document, path);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kClassExt: {
        const auto classExt = reinterpret_cast<const hadron::parse::ClassExtNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("ClassExt"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"methods", sizeof("methods") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(classExt->methods.get(), document, path);
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
        serializeParseNode(classNode->variables.get(), document, path);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"methods", sizeof("methods") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(classNode->methods.get(), document, path);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kReturn: {
        const auto retNode = reinterpret_cast<const hadron::parse::ReturnNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("ReturnNode"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"valueExpr", sizeof("valueExpr") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(retNode->valueExpr.get(), document, path);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kDynList: {
        const auto dynList = reinterpret_cast<const hadron::parse::DynListNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("DynList"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"elements", sizeof("elements") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(dynList->elements.get(), document, path);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kBlock: {
        const auto block = reinterpret_cast<const hadron::parse::BlockNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Block"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"arguments", sizeof("arguments") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(block->arguments.get(), document, path);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"variables", sizeof("variables") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(block->variables.get(), document, path);
        path.pop_back();
        path.emplace_back(rapidjson::Pointer::Token{"body", sizeof("body") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(block->body.get(), document, path);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kLiteral: {
        const auto literal = reinterpret_cast<const hadron::parse::LiteralNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("Literal"), document.GetAllocator());
        /* auto& value = */ jsonNode.AddMember("value", rapidjson::Value(), document.GetAllocator());
        // serializeSlot(literal->value, value, document);
        path.emplace_back(rapidjson::Pointer::Token{"blockLiteral", sizeof("blockLiteral") - 1,
                rapidjson::kPointerInvalidIndex});
        serializeParseNode(literal->blockLiteral.get(), document, path);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kName: {
    } break;
    case hadron::parse::NodeType::kExprSeq: {
        const auto exprSeq = reinterpret_cast<const hadron::parse::ExprSeqNode*>(node);
        jsonNode.AddMember("nodeType", rapidjson::Value("ExprSeq"), document.GetAllocator());
        path.emplace_back(rapidjson::Pointer::Token{"expr", sizeof("expr") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(exprSeq->expr.get(), document, path);
        path.pop_back();
    } break;
    case hadron::parse::NodeType::kAssign: {
    } break;
    case hadron::parse::NodeType::kSetter: {
    } break;
    case hadron::parse::NodeType::kKeyValue: {
    } break;
    case hadron::parse::NodeType::kCall: {
    } break;
    case hadron::parse::NodeType::kBinopCall: {
    } break;
    case hadron::parse::NodeType::kPerformList: {
    } break;
    case hadron::parse::NodeType::kNumericSeries: {
    } break;
    case hadron::parse::NodeType::kIf: {
    } break;
    }

    if (node->next) {
        path.emplace_back(rapidjson::Pointer::Token{"next", sizeof("next") - 1, rapidjson::kPointerInvalidIndex});
        serializeParseNode(node->next.get(), document, path);
        path.pop_back();
    }
}

void JSONTransport::JSONTransportImpl::serializeSlot(hadron::Slot slot, rapidjson::Value& target,
        rapidjson::Document& document) {
    target.SetObject();
    switch (slot.type) {
    case hadron::Type::kNil:
        target.AddMember("type", rapidjson::Value("nil"), document.GetAllocator());
        break;
    case hadron::Type::kInteger:
        target.AddMember("type", rapidjson::Value("integer"), document.GetAllocator());
        target.AddMember("value", rapidjson::Value(slot.value.intValue), document.GetAllocator());
        break;
    case hadron::Type::kFloat:
        target.AddMember("type", rapidjson::Value("float"), document.GetAllocator());
        target.AddMember("value", rapidjson::Value(slot.value.floatValue), document.GetAllocator());
        break;
    case hadron::Type::kBoolean:
        target.AddMember("type", rapidjson::Value("boolean"), document.GetAllocator());
        target.AddMember("value", rapidjson::Value(slot.value.boolValue), document.GetAllocator());
        break;
    case hadron::Type::kString:
    case hadron::Type::kSymbol:
    case hadron::Type::kClass:
    case hadron::Type::kObject:
    case hadron::Type::kArray:
    default:
        break;
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

void JSONTransport::sendParseTree(lsp::ID id, const hadron::parse::Node* node) {
    m_impl->sendParseTree(id, node);
}

} // namespace server