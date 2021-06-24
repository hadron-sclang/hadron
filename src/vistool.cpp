#include "FileSystem.hpp"

#include "ErrorReporter.hpp"
#include "Literal.hpp"
#include "Parser.hpp"
#include "SymbolTable.hpp"
#include "Type.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <fstream>
#include <memory>
#include <string_view>

DEFINE_string(inputFile, "", "path to input file to process");
DEFINE_bool(parseTree, false, "print the parse tree");
DEFINE_string(outputFile, "", "path to output file to save to");

std::string nullOrNo(const hadron::parse::Node* node) {
    if (node) return "";
    return "<i>null</i>";
}

std::string trueFalse(bool v) {
    if (v) return "true";
    return "false";
}

std::string printLiteral(const hadron::Literal& literal) {
    switch (literal.type()) {
        case hadron::Type::kNil:
            return "(nil)";

        case hadron::Type::kInteger:
            return fmt::format("(int) {}", literal.asInteger());

        case hadron::Type::kFloat:
            return fmt::format("(float) {}", literal.asFloat());

        case hadron::Type::kBoolean:
            return fmt::format("(bool) {}", trueFalse(literal.asBoolean()));

        case hadron::Type::kString:
            return "(string)";

        case hadron::Type::kSymbol:
            return fmt::format("(symbol) hash: {:x}", literal.asSymbolHash());

        case hadron::Type::kClass:
            return "(class)";
        
        case hadron::Type::kObject:
            return "(object)";

        case hadron::Type::kArray:
            return "(array)";

        case hadron::Type::kAny:
            return "(any)";
    }

    return "(unknown type!)";
}

void visualizeParseNode(std::ofstream& outFile, hadron::Parser& parser, int& serial, const hadron::parse::Node* node) {
    auto token = parser.tokens()[node->tokenIndex];
    int nodeSerial = serial;
    ++serial;

    // Generally the format of the labels is a table:
    //    top row is node type in bold, then the next pointer
    //    next row is the token in monospace
    //    next (optional) row are any non-Node pointer member variables
    //    last row is all node pointer members (excluding next)
    // This allows for the trees to build down from root nodes with siblings extending off to the right
    switch (node->nodeType) {
    case hadron::parse::NodeType::kEmpty:
        spdlog::error("Parser returned Empty node!");
        return;
    
    case hadron::parse::NodeType::kVarDef: {
        const auto varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>VarDef</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td>varName: {}</td></tr>"
            "<tr><td>hasReadAccessor: {}</td></tr>"
            "<tr><td>hasWriteAccessor: {}</td></tr>"
            "<tr><td port=\"initialValue\">initialValue {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            std::string(varDef->varName.data(), varDef->varName.size()),
            trueFalse(varDef->hasReadAccessor),
            trueFalse(varDef->hasWriteAccessor),
            nullOrNo(varDef->initialValue.get()));
        if (varDef->initialValue) {
            outFile << fmt::format("    node_{}:initialValue -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, varDef->initialValue.get());
        }
    } break;

    case hadron::parse::NodeType::kVarList: {
        const auto varList = reinterpret_cast<const hadron::parse::VarListNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>VarList</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td port=\"definitions\">definitions {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            nullOrNo(varList->definitions.get()));
        if (varList->definitions) {
            outFile << fmt::format("    node_{}:definitions -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, varList->definitions.get());
        }
    } break;

    case hadron::parse::NodeType::kArgList: {
        const auto argList = reinterpret_cast<const hadron::parse::ArgListNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>ArgList</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td>varArgsName: {}</td></tr>"
            "<tr><td port=\"varList\">varList {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            std::string(argList->varArgsName.data(), argList->varArgsName.size()),
            nullOrNo(argList->varList.get()));
        if (argList->varList) {
            outFile << fmt::format("    node_{}:varList -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, argList->varList.get());
        }     
    } break;

    case hadron::parse::NodeType::kMethod: {
        const auto method = reinterpret_cast<const hadron::parse::MethodNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Method</b></td>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td>methodName: {}</td></tr>"
            "<tr><td>isClassMethod: {}</td></tr>"
            "<tr><td>primitive: {}</td></tr>"
            "<tr><td port=\"arguments\">arguments {}</td></tr>"
            "<tr><td port=\"variables\">variables {}</td></tr>"
            "<tr><td port=\"body\">body {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            std::string(method->methodName.data(), method->methodName.size()),
            trueFalse(method->isClassMethod),
            std::string(method->primitive.data(), method->primitive.size()),
            nullOrNo(method->arguments.get()),
            nullOrNo(method->variables.get()),
            nullOrNo(method->body.get()));
        if (method->arguments) {
            outFile << fmt::format("    node_{}:arguments -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, method->arguments.get());
        }            
        if (method->variables) {
            outFile << fmt::format("    node_{}:variables -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, method->variables.get());
        }
        if (method->body) {
            outFile << fmt::format("    node_{}:body -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, method->body.get());
        }
    } break;

    case hadron::parse::NodeType::kClassExt:
        spdlog::warn("TODO: ClassExtNode");
        break;

    case hadron::parse::NodeType::kClass:
        spdlog::warn("TODO: ClassNode");
        break;

    case hadron::parse::NodeType::kReturn:
        spdlog::warn("TODO: ReturnNode");
        break;

    case hadron::parse::NodeType::kDynList:
        spdlog::warn("TODO: DynListNode");
        break;

    case hadron::parse::NodeType::kBlock: {
        const auto block = reinterpret_cast<const hadron::parse::BlockNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Block</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td port=\"arguments\">arguments {}</td></tr>"
            "<tr><td port=\"variables\">variables {}</td></tr>"
            "<tr><td port=\"body\">body {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            nullOrNo(block->arguments.get()),
            nullOrNo(block->variables.get()),
            nullOrNo(block->body.get()));
        if (block->arguments) {
            outFile << fmt::format("    node_{}:arguments -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, block->arguments.get());
        }            
        if (block->variables) {
            outFile << fmt::format("    node_{}:variables -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, block->variables.get());
        }
        if (block->body) {
            outFile << fmt::format("    node_{}:body -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, block->body.get());
        }
    } break;

    case hadron::parse::NodeType::kValue: {
        const auto value = reinterpret_cast<const hadron::parse::ValueNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Value</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td>value: {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            printLiteral(value->value)); 
    } break;

    case hadron::parse::NodeType::kLiteral: {
        const auto literal = reinterpret_cast<const hadron::parse::LiteralNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Literal</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td>value: {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            printLiteral(literal->value)); 
    } break;

    case hadron::parse::NodeType::kName: {
        const auto name = reinterpret_cast<const hadron::parse::NameNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Name</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td>name: {}</td></tr>"
            "<tr><td>isGlobal: {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            std::string(name->name.data(), name->name.size()),
            trueFalse(name->isGlobal)); 
    } break;

    case hadron::parse::NodeType::kExprSeq: {
        const auto exprSeq = reinterpret_cast<const hadron::parse::ExprSeqNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>ExprSeq</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td port=\"expr\">expr {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            nullOrNo(exprSeq->expr.get()));
        if (exprSeq->expr) {
            outFile << fmt::format("    node_{}:expr -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, exprSeq->expr.get());
        }            
    } break;

    case hadron::parse::NodeType::kAssign: {
        const auto assign = reinterpret_cast<const hadron::parse::AssignNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Assign</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td port=\"name\">name {}</td></tr>"
            "<tr><td port=\"value\">value {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            nullOrNo(assign->name.get()),
            nullOrNo(assign->value.get()));
        if (assign->name) {
            outFile << fmt::format("    node_{}:name -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, assign->name.get());
        }            
        if (assign->value) {
            outFile << fmt::format("    node_{}:value -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, assign->value.get());
        }
    } break;

    case hadron::parse::NodeType::kSetter:
        spdlog::warn("TODO: SetterNode");
        break;

    case hadron::parse::NodeType::kKeyValue:
        spdlog::warn("TODO: KeyValueNode");
        break;

    case hadron::parse::NodeType::kCall: {
        const auto call = reinterpret_cast<const hadron::parse::CallNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Call</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td>selector '{}'</td></tr>"
            "<tr><td port=\"target\">target {}</td></tr>"
            "<tr><td port=\"arguments\">arguments {}</td></tr>"
            "<tr><td port=\"keywordArguments\">keywordArguments {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            parser.symbolTable()->getSymbol(call->selector),
            nullOrNo(call->target.get()),
            nullOrNo(call->arguments.get()),
            nullOrNo(call->keywordArguments.get()));
        if (call->target) {
            outFile << fmt::format("    node_{}:target -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, call->target.get());
        }
        if (call->arguments) {
            outFile << fmt::format("    node_{}:arguments -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, call->arguments.get());
        }            
        if (call->keywordArguments) {
            outFile << fmt::format("    node_{}:keywordArguments -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, call->keywordArguments.get());
        }
    } break;

    case hadron::parse::NodeType::kBinopCall: {
        const auto binopCall = reinterpret_cast<const hadron::parse::BinopCallNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>BinopCall</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td>selector: '{}'</td></tr>"
            "<tr><td port=\"leftHand\">leftHand {}</td></tr>"
            "<tr><td port=\"rightHand\">rightHand {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            parser.symbolTable()->getSymbol(binopCall->selector),
            nullOrNo(binopCall->leftHand.get()),
            nullOrNo(binopCall->rightHand.get()));
        if (binopCall->leftHand) {
            outFile << fmt::format("    node_{}:leftHand -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, binopCall->leftHand.get());
        }
        if (binopCall->rightHand) {
            outFile << fmt::format("    node_{}:rightHand -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, binopCall->rightHand.get());
        }            
    } break;

    case hadron::parse::NodeType::kPerformList:
        spdlog::warn("TODO: PerformListNode");
        break;

    case hadron::parse::NodeType::kNumericSeries:
        spdlog::warn("TODO: NumericSeriesNode");
        break;

    default:
        spdlog::error("Encountered unknown parse tree node type."); 
        return;
    }

    if (node->next) {
        outFile << fmt::format("    node_{}:next -> node_{}\n", nodeSerial, serial);
        visualizeParseNode(outFile, parser, serial, node->next.get());
    }
}

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    spdlog::set_level(spdlog::level::debug);

    fs::path filePath(FLAGS_inputFile);
    if (!fs::exists(filePath)) {
        spdlog::error("File '{}' does not exist.", filePath.string());
        return -1;
    }

    auto fileSize = fs::file_size(filePath);
    auto fileContents = std::make_unique<char[]>(fileSize);
    std::ifstream inFile(filePath);
    if (!inFile) {
        spdlog::error("Failed to open input file {}", filePath.string());
        return -1;
    }
    inFile.read(fileContents.get(), fileSize);
    if (!inFile) {
        spdlog::error("Failed to read file {}", filePath.string());
        return -1;
    }

    if (FLAGS_parseTree) {
        auto errorReporter = std::make_shared<hadron::ErrorReporter>();
        hadron::Parser parser(std::string_view(fileContents.get(), fileSize), errorReporter);
        if (!parser.parse()) {
            spdlog::error("Failed to parse file {}", filePath.string());
            return -1;
        }

        std::ofstream outFile(FLAGS_outputFile);
        if (!outFile) {
            spdlog::error("Failed to open output file {}", FLAGS_outputFile);
        }

        outFile << fmt::format("// parse tree visualization of {}\n", FLAGS_inputFile);
        outFile << "digraph HadronAST {" << std::endl;
        int serial = 0;
        visualizeParseNode(outFile, parser, serial, parser.root());    
        outFile << "}" << std::endl;
    }

    return 0;
}
