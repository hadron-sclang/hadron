// vistool generates dot files, suitable for consumption with graphviz, of different outputs of the Hadron compiler
// processess. It currently can generate parse tree and Control Flow Graph (CFG) block tree visualizations.
#include "FileSystem.hpp"

#include "ErrorReporter.hpp"
#include "HIR.hpp"
#include "CodeGenerator.hpp"
#include "Literal.hpp"
#include "Parser.hpp"
#include "SymbolTable.hpp"
#include "Type.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <fstream>
#include <memory>
#include <string_view>
#include <variant>

DEFINE_string(inputFile, "", "path to input file to process");
DEFINE_string(outputFile, "", "path to output file to save to");
DEFINE_bool(parseTree, false, "print the parse tree");
DEFINE_bool(blockTree, false, "print the block tree");

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
            return "(symbol)";

        case hadron::Type::kClass:
            return "(class)";

        case hadron::Type::kObject:
            return "(object)";

        case hadron::Type::kArray:
            return "(array)";

        case hadron::Type::kSlot:
            return "(slot)";
    }

    return "(unknown type!)";
}

std::string htmlEscape(std::string_view view) {
    std::string escaped;
    for (size_t i = 0; i < view.size(); ++i) {
        switch (view[i]) {
            case '(':
                escaped += "&#40;";
                break;

            case ')':
                escaped += "&#41;";
                break;

            case '&':
                escaped += "&amp;";
                break;

            case '<':
                escaped += "&lt;";
                break;

            case '>':
                escaped += "&gt;";
                break;

            case '\n':
                escaped += "<br/>";
                break;

            case '\'':
                escaped += "&apos;";
                break;

            case '"':
                escaped += "&quot;";
                break;

            default:
                escaped += view[i];
                break;
        }
    }
    return escaped;
}

void visualizeParseNode(std::ofstream& outFile, hadron::Parser& parser, int& serial, const hadron::parse::Node* node) {
    auto token = parser.tokens()[node->tokenIndex];
    int nodeSerial = serial;
    ++serial;
    // Make an edge in gray from this parse tree node to the token in the code subgraph that it relates to.
    outFile << fmt::format("    node_{} -> line_{}:token_{} [color=darkGray]\n", nodeSerial,
        parser.errorReporter()->getLineNumber(token.range.data()), node->tokenIndex);

    // Generally the format of the labels is a vertically stacked table:
    //    top row is node type in bold
    //    then the next pointer
    //    next row is the token in monospace
    //    next (optional) rows are any non-Node pointer member variables
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
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td>hasReadAccessor: {}</td></tr>"
            "<tr><td>hasWriteAccessor: {}</td></tr>"
            "<tr><td port=\"initialValue\">initialValue {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
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
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
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
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td port=\"varList\">varList {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            nullOrNo(argList->varList.get()));
        if (argList->varList) {
            outFile << fmt::format("    node_{}:varList -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, argList->varList.get());
        }
    } break;

    case hadron::parse::NodeType::kMethod: {
        const auto method = reinterpret_cast<const hadron::parse::MethodNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Method</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td>isClassMethod: {}</td></tr>"
            "<tr><td port=\"arguments\">arguments {}</td></tr>"
            "<tr><td port=\"variables\">variables {}</td></tr>"
            "<tr><td port=\"body\">body {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            htmlEscape(std::string(token.range.data(), token.range.size())),
            trueFalse(method->isClassMethod),
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

    case hadron::parse::NodeType::kClass: {
        const auto classNode = reinterpret_cast<const hadron::parse::ClassNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Class</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td port=\"variables\">variables {}</td></tr>"
            "<tr><td port=\"methods\">methods {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            nullOrNo(classNode->variables.get()),
            nullOrNo(classNode->methods.get()));
        if (classNode->variables) {
            outFile << fmt::format("    node_{}:variables -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, classNode->variables.get());
        }
        if (classNode->methods) {
            outFile << fmt::format("    node_{}:methods -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, classNode->methods.get());
        }
    } break;

    case hadron::parse::NodeType::kReturn: {
        const auto returnNode = reinterpret_cast<const hadron::parse::ReturnNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Return</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td port=\"valueExpr\">valueExpr {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            nullOrNo(returnNode->valueExpr.get()));
        if (returnNode->valueExpr) {
            outFile << fmt::format("    node_{}:valueExpr -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, returnNode->valueExpr.get());
        }
    } break;

    case hadron::parse::NodeType::kDynList:
        spdlog::warn("TODO: DynListNode");
        break;

    case hadron::parse::NodeType::kBlock: {
        const auto block = reinterpret_cast<const hadron::parse::BlockNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Block</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
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

    case hadron::parse::NodeType::kLiteral: {
        const auto literal = reinterpret_cast<const hadron::parse::LiteralNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Literal</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
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
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td>isGlobal: {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            std::string(token.range.data(), token.range.size()),
            trueFalse(name->isGlobal)); 
    } break;

    case hadron::parse::NodeType::kExprSeq: {
        const auto exprSeq = reinterpret_cast<const hadron::parse::ExprSeqNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>ExprSeq</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
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
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
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
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td port=\"target\">target {}</td></tr>"
            "<tr><td port=\"arguments\">arguments {}</td></tr>"
            "<tr><td port=\"keywordArguments\">keywordArguments {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            htmlEscape(std::string(token.range.data(), token.range.size())),
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
            "<tr><td port=\"token\"><font face=\"monospace\">{}</font></td></tr>"
            "<tr><td port=\"leftHand\">leftHand {}</td></tr>"
            "<tr><td port=\"rightHand\">rightHand {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            htmlEscape(std::string(token.range.data(), token.range.size())),
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

std::string printOperand(const hadron::HIR::Operand& operand) {
    if (std::holds_alternative<hadron::ValueRef>(operand)) {
        const hadron::ValueRef& valueRef = std::get<hadron::ValueRef>(operand);
        if (valueRef.isBlockValue) {
            return fmt::format("<i>block<sub>{}</sub></i>", valueRef.block->id);
        }
        return std::string(valueRef.name.data(), valueRef.name.size());
    } else if (std::holds_alternative<hadron::Literal>(operand)) {
        return printLiteral(std::get<hadron::Literal>(operand));
    }
    return "&lt;unknown operand&gt;";
}

std::string printHIR(const hadron::HIR& hir) {
    std::string hirString;
    switch (hir.opcode) {
    case hadron::kLessThanI32:
        hirString = fmt::format("{} &#8592; {} &lt; {}", printOperand(hir.operands[0]), printOperand(hir.operands[1]),
            printOperand(hir.operands[2]));
        break;

    case hadron::Opcode::kAssignI32:
        hirString = fmt::format("{} &#8592; {}", printOperand(hir.operands[0]), printOperand(hir.operands[1]));
        break;

    default:
        break;
    }

    return hirString;
}

void visualizeBlockNode(std::ofstream& outFile, const hadron::Block* block) {
    // The format for blocks is a vertical table first with Values in the scope for this block, just a list of comma
    // separated names in a cell. The next cell is vertically stacked instructions. Control flow arrows are drawn in
    // black, scoping arrows drawn in gray.
    if (block->scopeParent) {
        outFile << fmt::format("    block_{} -> block_{} [color=darkGray]\n", block->id, block->scopeParent->id);
    }

    outFile << fmt::format("    block_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" "
    "cellspacing=\"0\">\n", block->id);
    outFile << "        <tr><td><font face=\"monospace\">&nbsp;";
    for (const auto& v : block->values) {
        outFile << fmt::format("{} ", v.second.name);
    }
    outFile << "</font></td></tr>" << std::endl;
    for (const auto& hir : block->instructions) {
        outFile << fmt::format("        <tr><td sides=\"LR\">{}</td></tr>\n", printHIR(hir));
    }
    outFile << "        <tr><td sides=\"LRB\"></td></tr></table>>]" << std::endl;

    // Control flow arrows in black
    for (const auto& exit : block->exits) {
        outFile << fmt::format("    block_{} -> block_{}\n", block->id, exit->id);
    }

    for (const auto& child : block->scopeChildren) {
        visualizeBlockNode(outFile, child.get());
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

    if (FLAGS_parseTree) {
        outFile << fmt::format("// parse tree visualization of {}\n", FLAGS_inputFile);
        outFile << "digraph HadronParseTree {" << std::endl;
        outFile << "    subgraph {" << std::endl;
        outFile << "        edge [style=\"invis\"]" << std::endl;
        size_t currentLine = 1;
        size_t tokenIndex = 0;
        while (tokenIndex < parser.tokens().size()) {
            outFile << fmt::format("        line_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" "
                "cellspacing=\"0\"><tr><td><font point-size=\"24\">line {}:</font></td>", currentLine, currentLine);
            size_t tokenLine = errorReporter->getLineNumber(parser.tokens()[tokenIndex].range.data());
            int tokenLineCount = 0;
            while (tokenLine == currentLine) {
                outFile << fmt::format("<td port=\"token_{}\"><font face=\"monospace\" "
                    "point-size=\"24\">{}</font></td>", tokenIndex,
                    htmlEscape(std::string(parser.tokens()[tokenIndex].range.data(),
                        parser.tokens()[tokenIndex].range.size())));
                ++tokenLineCount;
                ++tokenIndex;
                if (tokenIndex < parser.tokens().size()) {
                    tokenLine = errorReporter->getLineNumber(parser.tokens()[tokenIndex].range.data());
                } else {
                    tokenLine = 0;
                }
            }
            outFile << "</tr></table>>]" << std::endl;
            if (tokenLine > 1 && tokenLineCount > 0) {
                outFile << fmt::format("        line_{} -> line_{}\n", currentLine, tokenLine);
            }
            currentLine = tokenLine;
        }
        outFile << "    }  // end of code subgraph" << std::endl;
        int serial = 0;
        visualizeParseNode(outFile, parser, serial, parser.root());
        outFile << "}" << std::endl;
    } else if (FLAGS_blockTree) {
        hadron::CodeGenerator codeGen;
        auto block = codeGen.buildBlock(reinterpret_cast<const hadron::parse::BlockNode*>(parser.root()), nullptr);

        outFile << fmt::format("// block tree visualization of {}\n", FLAGS_inputFile);
        outFile << "digraph HadronBlockTree {" << std::endl;
        visualizeBlockNode(outFile, block.get());
        outFile << "}" << std::endl;
    }

    return 0;
}
