#include "server/HadronServer.hpp"

#include "hadron/Arch.hpp"
#include "hadron/AST.hpp"
#include "hadron/ASTBuilder.hpp"
#include "hadron/Block.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/ClassLibrary.hpp"
#include "hadron/Emitter.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/Parser.hpp"
#include "hadron/RegisterAllocator.hpp"
#include "hadron/Resolver.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/Scope.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/VirtualJIT.hpp"
#include "server/JSONTransport.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

namespace server {

HadronServer::HadronServer(std::unique_ptr<JSONTransport> jsonTransport):
        m_jsonTransport(std::move(jsonTransport)),
        m_state(kUninitialized),
        m_errorReporter(std::make_shared<hadron::ErrorReporter>()),
        m_runtime(std::make_unique<hadron::Runtime>(m_errorReporter)) {
    m_jsonTransport->setServer(this);
}

int HadronServer::runLoop() {
    return m_jsonTransport->runLoop();
}

void HadronServer::initialize(std::optional<lsp::ID> id) {
    if (!m_runtime->initInterpreter()) {
        m_jsonTransport->sendErrorResponse(id, JSONTransport::ErrorCode::kInternalError,
                "Failed to initialize Hadron runtime.");
        return;
    }

    m_state = kRunning;
    m_jsonTransport->sendInitializeResult(id);
}

void HadronServer::semanticTokensFull(const std::string& filePath) {
    hadron::SourceFile sourceFile(filePath);
    if (!sourceFile.read(m_errorReporter)) {
        m_jsonTransport->sendErrorResponse(std::nullopt, JSONTransport::ErrorCode::kFileReadError,
                fmt::format("Failed to read file {} for lexing.", filePath));
        return;
    }

    auto code = sourceFile.codeView();
    hadron::Lexer lexer(code, m_errorReporter);
    if (!lexer.lex() || !m_errorReporter->ok()) {
        // TODO: error reporting
        return;
    }

    m_jsonTransport->sendSemanticTokens(lexer.tokens());
}

void HadronServer::hadronCompilationDiagnostics(lsp::ID id, const std::string& filePath) {
    hadron::SourceFile sourceFile(filePath);
    if (!sourceFile.read(m_errorReporter)) {
        m_jsonTransport->sendErrorResponse(std::nullopt, JSONTransport::ErrorCode::kFileReadError,
                fmt::format("Failed to read file {} for parsing.", filePath));
        return;
    }

    auto code = sourceFile.codeView();
    auto lexer = std::make_shared<hadron::Lexer>(code, m_errorReporter);
    if (!lexer->lex() || !m_errorReporter->ok()) {
        // TODO: errorReporter starts reporting problems itself
        return;
    }

    // This is not a good way to determine "classNess" of a file. The best fix is likely going to
    // be to restructure the SC grammar to be able to mix class definitions and interpreted code
    // more freely. A better medium-term fix is likely going to be adjusting the protocol to
    // better clarify user intent - are they wanting to see a class file dump or a script dump?
    // But for now, as these APIs are still very plastic, we key off of file extension.
    bool isClassFile = filePath.size() > 3 && (filePath.substr(filePath.size() - 3, 3) == ".sc");

    auto parser = std::make_shared<hadron::Parser>(lexer.get(), m_errorReporter);
    std::vector<CompilationUnit> units;

    // Determine if the input file was an interpreter script or a class file and parse accordingly.
    if (isClassFile) {
        if (!parser->parseClass() || !m_errorReporter->ok()) {
            // TODO: error handling
            return;
        }
        const hadron::parse::Node* node = parser->root();
        while (node) {
            auto className = hadron::library::Symbol::fromView(m_runtime->context(),
                    lexer->tokens()[node->tokenIndex].range);

            auto classDef = m_runtime->context()->classLibrary->findClassNamed(className);
            assert(!classDef.isNil());

            const hadron::parse::MethodNode* methodNode = nullptr;
            if (node->nodeType == hadron::parse::NodeType::kClass) {
                auto classNode = reinterpret_cast<const hadron::parse::ClassNode*>(node);
                methodNode = classNode->methods.get();
            } else if (node->nodeType == hadron::parse::NodeType::kClassExt) {
                auto classExtNode = reinterpret_cast<const hadron::parse::ClassExtNode*>(node);
                methodNode = classExtNode->methods.get();
            }

            while (methodNode) {
                auto methodName = hadron::library::Symbol::fromView(m_runtime->context(),
                        lexer->tokens()[methodNode->tokenIndex].range);

                hadron::library::Method methodDef;
                for (int32_t i = 0; i < classDef.methods().size(); ++i) {
                    auto method = classDef.methods().typedAt(i);
                    if (method.name(m_runtime->context()) == methodName) {
                        methodDef = method;
                        break;
                    }
                }
                assert(!methodDef.isNil());

                addCompilationUnit(methodDef, lexer, parser, methodNode->body.get(), units);
                methodNode = reinterpret_cast<const hadron::parse::MethodNode*>(methodNode->next.get());
            }

            node = node->next.get();
        }
    } else {
        if (!parser->parse() || !m_errorReporter->ok()) {
            // TODO: errors
            return;
        }
        assert(parser->root()->nodeType == hadron::parse::NodeType::kBlock);

        addCompilationUnit(m_runtime->context()->classLibrary->interpreterContext(), lexer, parser,
                reinterpret_cast<const hadron::parse::BlockNode*>(parser->root()), units);
    }
    m_jsonTransport->sendCompilationDiagnostics(m_runtime->context(), id, units);
}

void HadronServer::addCompilationUnit(hadron::library::Method methodDef, std::shared_ptr<hadron::Lexer> lexer,
        std::shared_ptr<hadron::Parser> parser, const hadron::parse::BlockNode* blockNode,
        std::vector<CompilationUnit>& units) {
    std::string name(methodDef.name(m_runtime->context()).view(m_runtime->context()));

    SPDLOG_TRACE("Compile Diagnostics AST Builder {}", name);
    hadron::ASTBuilder astBuilder(m_errorReporter);
    auto blockAST = astBuilder.buildBlock(m_runtime->context(), lexer.get(), blockNode);

    SPDLOG_TRACE("Compile Diagnostics Block Builder {}", name);
    hadron::BlockBuilder blockBuilder(m_errorReporter);
    auto frame = blockBuilder.buildMethod(m_runtime->context(), methodDef, blockAST.get());

    SPDLOG_TRACE("Compile Diagnostics Block Serializer {}", name);
    hadron::BlockSerializer blockSerializer;
    auto linearFrame = blockSerializer.serialize(frame.get());

    SPDLOG_TRACE("Compile Diagnostics Lifetime Analyzer {}", name);
    hadron::LifetimeAnalyzer lifetimeAnalyzer;

    lifetimeAnalyzer.buildLifetimes(linearFrame.get());

    SPDLOG_TRACE("Compile Diagnostics Register Allocator {}", name);
    hadron::RegisterAllocator registerAllocator(hadron::kNumberOfPhysicalRegisters);
    registerAllocator.allocateRegisters(linearFrame.get());

    SPDLOG_TRACE("Compile Diagnostics Resolver {}", name);
    hadron::Resolver resolver;
    resolver.resolve(linearFrame.get());

    SPDLOG_TRACE("Compile Diagnostics Emitter {}", name);
    hadron::Emitter emitter;
    hadron::VirtualJIT jit;
    size_t byteCodeSize = linearFrame->instructions.size() * 16;
    auto byteCode = std::make_unique<int8_t[]>(byteCodeSize);
    jit.begin(byteCode.get(), byteCodeSize);
    emitter.emit(linearFrame.get(), &jit);
    size_t finalSize = 0;
    jit.end(&finalSize);
    assert(finalSize < byteCodeSize);

    SPDLOG_TRACE("Compile Diagnostics Rebuilding Block {}", name);

    units.emplace_back(CompilationUnit{name, lexer, parser, blockNode, std::move(blockAST), std::move(frame),
            std::move(linearFrame), std::move(byteCode), finalSize});
}

} // namespace server