// vistool generates dot files, suitable for consumption with graphviz, of different outputs of the Hadron compiler
// processess. It currently can generate parse tree and Control Flow Graph (CFG) block tree visualizations.
#include "FileSystem.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Hash.hpp"
#include "hadron/HIR.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Slot.hpp"
#include "hadron/SSABuilder.hpp"
#include "hadron/Type.hpp"
#include "Keywords.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <vector>

DEFINE_string(inputFile, "", "path to input file to process");
DEFINE_string(outputFile, "", "path to output file to save to");
DEFINE_bool(parseTree, false, "print the parse tree");
DEFINE_bool(ssa, false, "print the SSA tree");

std::string nullOrNo(const hadron::parse::Node* node) {
    if (node) return "";
    return "<i>null</i>";
}

std::string trueFalse(bool v) {
    if (v) return "true";
    return "false";
}

std::string printType(const uint32_t type) {
    switch (type) {
        case hadron::Type::kNil:
            return "(nil)";

        case hadron::Type::kInteger:
            return "(int)";

        case hadron::Type::kFloat:
            return "(float)";

        case hadron::Type::kBoolean:
            return "(bool)";

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

        case hadron::Type::kAny:
            return "(*any*)";

        case hadron::Type::kMachineCodePointer:
            return "(machine code)";

        case hadron::Type::kFramePointer:
            return "(frame pointer)";

        case hadron::Type::kStackPointer:
            return "(stack pointer)";

        case hadron::Type::kType:
            return "(type)";
    }

    // Must be a compound type, build.
    std::vector<std::string> types;
    if (type & hadron::Type::kNil) {
        types.emplace_back("nil");
    }
    if (type & hadron::Type::kInteger) {
        types.emplace_back("int");
    }
    if (type & hadron::Type::kFloat) {
        types.emplace_back("float");
    }
    if (type & hadron::Type::kBoolean) {
        types.emplace_back("bool");
    }
    if (type & hadron::Type::kString) {
        types.emplace_back("string");
    }
    if (type & hadron::Type::kSymbol) {
        types.emplace_back("symbol");
    }
    if (type & hadron::Type::kClass) {
        types.emplace_back("class");
    }
    if (type & hadron::Type::kObject) {
        types.emplace_back("object");
    }
    if (type & hadron::Type::kArray) {
        types.emplace_back("array");
    }

    if (types.size() == 0) {
        return "(unknown type!)";
    }
    assert(types.size() > 2);
    std::string compoundType = "(" + types[0];
    for (size_t i = 1; i < types.size() - 1; ++i) {
        compoundType += "| " + types[i];
    }
    compoundType += "| " + types[types.size() - 1] + ")";
    return compoundType;
}

std::string printSlot(const hadron::Slot& literal) {
    switch (literal.type) {
    case hadron::Type::kInteger:
        return fmt::format("(int) {}", literal.value.intValue);

    case hadron::Type::kFloat:
        return fmt::format("(float) {}", literal.value.floatValue);

    case hadron::Type::kBoolean:
        return fmt::format("(bool) {}", trueFalse(literal.value.boolValue));

    case hadron::Type::kType:
        return fmt::format("(type) {}", printType(literal.value.typeValue));

    default:
        break;
    }

    std::string type = printType(literal.type);
    return type;
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
    auto token = parser.lexer()->tokens()[node->tokenIndex];
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
            "<tr><td port=\"body\">body {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            htmlEscape(std::string(token.range.data(), token.range.size())),
            trueFalse(method->isClassMethod),
            nullOrNo(method->body.get()));
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
            printSlot(literal->value)); 
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

    case hadron::parse::NodeType::kIf: {
        const auto ifNode = reinterpret_cast<const hadron::parse::IfNode*>(node);
        outFile << fmt::format("    node_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>if</b></td></tr>"
            "<tr><td port=\"next\">next {}</td></tr>"
            "<tr><td port=\"condition\">condition {}</td></tr>"
            "<tr><td port=\"true\">true {}</td></tr>"
            "<tr><td port=\"false\">false {}</td></tr></table>>]\n",
            nodeSerial,
            nullOrNo(node->next.get()),
            nullOrNo(ifNode->condition.get()),
            nullOrNo(ifNode->trueBlock.get()),
            nullOrNo(ifNode->falseBlock.get()));
        if (ifNode->condition) {
            outFile << fmt::format("    node_{}:condition -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, ifNode->condition.get());
        }
        if (ifNode->trueBlock) {
            outFile << fmt::format("    node_{}:true -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, ifNode->trueBlock.get());
        }
        if (ifNode->falseBlock) {
            outFile << fmt::format("    node_{}:false -> node_{}\n", nodeSerial, serial);
            visualizeParseNode(outFile, parser, serial, ifNode->falseBlock.get());
        }
    } break;

    default:
        spdlog::error("Encountered unknown parse tree node type."); 
        return;
    }

    if (node->next) {
        outFile << fmt::format("    node_{}:next -> node_{}\n", nodeSerial, serial);
        visualizeParseNode(outFile, parser, serial, node->next.get());
    }
}

std::string printHash(hadron::Hash hash) {
    switch (hash) {
    case hadron::kAddHash:
        return "+";

    case hadron::kAssignHash:
        return "=";

    case hadron::kDivideHash:
        return "/";

    case hadron::kEqualToHash:
        return "==";

    case hadron::kExactlyEqualToHash:
        return "===";

    case hadron::kGreaterThanHash:
        return ">";

    case hadron::kGreaterThanOrEqualToHash:
        return ">=";

    case hadron::kIfHash:
        return "if";

    case hadron::kLeftArrowHash:
        return "<-";

    case hadron::kLessThanHash:
        return "<";

    case hadron::kLessThanOrEqualToHash:
        return "<=";

    case hadron::kModuloHash:
        return "%";

    case hadron::kMultiplyHash:
        return "*";

    case hadron::kNotEqualToHash:
        return "!=";

    case hadron::kNotExactlyEqualToHash:
        return "!==";

    case hadron::kPipeHash:
        return "|";

    case hadron::kReadWriteHash:
        return "<>";

    case hadron::kSubtractHash:
        return "-";

    case hadron::kWhileHash:
        return "while";
    }

    return fmt::format("hash {:16x} not found!", hash);
}

std::string findSymbol(hadron::Hash hash, const std::map<hadron::Hash, std::string>& symbols) {
    const auto lookup = symbols.find(hash);
    if (lookup != symbols.end()) {
        return lookup->second;
    }
    return fmt::format("hash {:16x}", hash);
}

std::string printValue(hadron::Value v) {
    if (!v.isValid()) {
        return ("&lt;invalid value&gt;");
    }
    return fmt::format("{} v<sub>{}</sub>", printType(v.typeFlags), v.number);
}

void visualizeBlock(std::ofstream& outFile, const hadron::Block* block) {
    outFile << fmt::format("    block_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n"
        "      <tr><td bgcolor=\"lightGray\"><b>Block {}</b></td></tr>\n", block->number, block->number);
    for (const auto& phi : block->phis) {
        outFile << fmt::format("      <tr><td>{} &#8592; &phi;(", printValue(phi->value));
        if (phi->inputs.size() > 1) {
            for (size_t i = 0; i < phi->inputs.size() - 1; ++i) {
                outFile << printValue(phi->inputs[i]) << ", ";
            }
        }
        if (phi->inputs.size() > 0) {
            outFile << printValue(phi->inputs[phi->inputs.size() - 1]);
        }
        outFile << ")</td></tr>" << std::endl;
    }

    for (const auto& hir : block->statements) {
        switch (hir->opcode) {
        case hadron::hir::Opcode::kLoadArgument: {
            const auto loadArg = reinterpret_cast<const hadron::hir::LoadArgumentHIR*>(hir.get());
            outFile << fmt::format("        <tr><td>{} &#8592; LoadArg({})</td></tr>\n",
                printValue(loadArg->value), loadArg->index);
        } break;

        case hadron::hir::Opcode::kLoadArgumentType: {
            const auto loadArgType = reinterpret_cast<const hadron::hir::LoadArgumentTypeHIR*>(hir.get());
            outFile << fmt::format("        <tr><td>{} &#8592; LoadArgType({})</td></tr>\n",
                printValue(loadArgType->value), loadArgType->index);
        } break;

        case hadron::hir::Opcode::kConstant: {
            const auto constant = reinterpret_cast<const hadron::hir::ConstantHIR*>(hir.get());
            outFile << fmt::format("      <tr><td>{} &#8592; {}</td></tr>\n", printValue(constant->value),
                printSlot(constant->constant));
        } break;

        case hadron::hir::Opcode::kStoreReturn: {
            const auto storeReturn = reinterpret_cast<const hadron::hir::StoreReturnHIR*>(hir.get());
            outFile << fmt::format("      <tr><td>StoreReturn({},{})</td></tr>\n",
                printValue(storeReturn->returnValue.first), printValue(storeReturn->returnValue.second));
        } break;

        case hadron::hir::Opcode::kResolveType: {
            const auto resolveType = reinterpret_cast<const hadron::hir::ResolveTypeHIR*>(hir.get());
            outFile << fmt::format("      <tr><td>{} &#8592; ResolveType({})</td></tr>\n",
                printValue(resolveType->value), printValue(resolveType->typeOfValue));
        } break;

        case hadron::hir::Opcode::kBranch:
        case hadron::hir::Opcode::kBranchIfZero:
        case hadron::hir::Opcode::kPhi:
            assert(false); // TODO
            break;
/*
        case hadron::hir::Opcode::kIf: {
            const auto ifHIR = reinterpret_cast<const hadron::hir::IfHIR*>(hir.get());
            outFile << fmt::format("      <tr><td>{} &#8592; if {},{} then goto {} else goto {}</td></tr>\n",
                printValue(ifHIR->value), printValue(ifHIR->condition.first), printValue(ifHIR->condition.second),
                ifHIR->trueBlock, ifHIR->falseBlock);
        } break;
*/
        case hadron::hir::Opcode::kLabel: {
            assert(false); // TODO
            break;
        }

        case hadron::hir::Opcode::kDispatchCall: {
            const auto dispatchCall = reinterpret_cast<const hadron::hir::DispatchCallHIR*>(hir.get());
            // Assumed to have at least one argument always.
            assert(dispatchCall->arguments.size());
            outFile << fmt::format("      <tr><td>{} &#8592; Dispatch({}", printValue(dispatchCall->value),
                printValue(dispatchCall->arguments[0]));
            for (size_t i = 1; i < dispatchCall->arguments.size(); ++i) {
                outFile << fmt::format(", v<sub>{}</sub>", dispatchCall->arguments[i].number);
            }
            for (size_t i = 0; i < dispatchCall->keywordArguments.size(); i += 2) {
                outFile << fmt::format(", v<sub>{}</sub>: v<sub>{}</sub>", dispatchCall->keywordArguments[i].number,
                    dispatchCall->keywordArguments[i + 1].number);
            }
            outFile << ")</td></tr>" << std::endl;
        } break;

        case hadron::hir::Opcode::kDispatchLoadReturn: {
            const auto dispatchRet = reinterpret_cast<const hadron::hir::DispatchLoadReturnHIR*>(hir.get());
            outFile << fmt::format("      <tr><td>{} &#8592; LoadReturn()</td></tr>\n", printValue(dispatchRet->value));
        } break;

        case hadron::hir::Opcode::kDispatchLoadReturnType: {
            const auto dispatchRetType = reinterpret_cast<const hadron::hir::DispatchLoadReturnTypeHIR*>(hir.get());
            outFile << fmt::format("      <tr><td>{} &#8592; LoadReturnType()</td></tr>\n",
                printValue(dispatchRetType->value));
        } break;

        case hadron::hir::Opcode::kDispatchCleanup: {
            outFile << "      <tr><td>DispatchCleanup()</td></tr>" << std::endl;
        } break;
        }
    }
    outFile << "      </table>>]" << std::endl;
}

void visualizeFrame(std::ofstream& outFile, int& serial, const hadron::Frame* frame) {
    int frameSerial = serial;
    ++serial;
    // Frames are subgraphs with cluster prefix name so that GraphViz will draw a box around them.
    outFile << fmt::format("  subgraph cluster_{} {{\n", frameSerial);
    for (const auto& block : frame->blocks) {
        visualizeBlock(outFile, block.get());
    }
    for (const auto& subFrame : frame->subFrames) {
        visualizeFrame(outFile, serial, subFrame.get());
    }
    outFile << fmt::format("  }}  // end of cluster_{}\n", frameSerial);
}

void visualizeFrameEdges(std::ofstream& outFile, const hadron::Frame* frame) {
    // Describe edges in our own blocks first.
    for (const auto& block : frame->blocks) {
        for (const auto successor : block->successors) {
            outFile << fmt::format("  block_{} -> block_{}\n", block->number, successor->number);
        }
    }
    for (const auto& subFrame : frame->subFrames) {
        visualizeFrameEdges(outFile, subFrame.get());
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
    // TODO: refactor me to use CompileContext
    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    errorReporter->setCode(fileContents.get());
    hadron::Lexer lexer(std::string_view(fileContents.get(), fileSize), errorReporter);
    if (!lexer.lex()) {
        spdlog::error("Failed to lex file {}", filePath.string());
        return -1;
    }
    hadron::Parser parser(&lexer, errorReporter);
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
        while (tokenIndex < parser.lexer()->tokens().size()) {
            std::string lineStart = fmt::format("        line_{} [shape=plain label=<<table border=\"0\" "
                "cellborder=\"1\" cellspacing=\"0\"><tr><td><font point-size=\"24\">line {}:</font></td>",
                currentLine, currentLine);
            size_t tokenLine = errorReporter->getLineNumber(parser.lexer()->tokens()[tokenIndex].range.data());
            int tokenLineCount = 0;
            std::string lineBody = "";
            while (tokenLine == currentLine) {
                lineBody += fmt::format("<td port=\"token_{}\"><font face=\"monospace\" "
                    "point-size=\"24\">{}</font></td>", tokenIndex,
                    htmlEscape(std::string(parser.lexer()->tokens()[tokenIndex].range.data(),
                        parser.lexer()->tokens()[tokenIndex].range.size())));
                ++tokenLineCount;
                ++tokenIndex;
                if (tokenIndex < parser.lexer()->tokens().size()) {
                    tokenLine = errorReporter->getLineNumber(parser.lexer()->tokens()[tokenIndex].range.data());
                } else {
                    tokenLine = 0;
                }
            }
            if (tokenLineCount) {
                outFile << lineStart << lineBody << "</tr></table>>]" << std::endl;
                if (tokenLine > 1) {
                    outFile << fmt::format("        line_{} -> line_{}\n", currentLine, tokenLine);
                }
            }
            currentLine = tokenLine;
        }
        outFile << "    }  // end of code subgraph" << std::endl;
        int serial = 0;
        visualizeParseNode(outFile, parser, serial, parser.root());
        outFile << "}" << std::endl;
    } else if (FLAGS_ssa) {
        hadron::SSABuilder builder(&lexer, errorReporter);
        // For now we only support Blocks at the top level.
        if (parser.root()->nodeType != hadron::parse::NodeType::kBlock) {
            spdlog::error("nonblock root, not building ssa");
            return -1;
        }
        auto frame = builder.buildFrame(reinterpret_cast<const hadron::parse::BlockNode*>(parser.root()));
        outFile << fmt::format("// parse tree visualization of {}\n", FLAGS_inputFile);
        outFile << "digraph HadronSSA {" << std::endl;
        int serial = 0;
        visualizeFrame(outFile, serial, frame.get());
        // Describing edges between blocks seems to cause dot to move blocks into subframes, so we describe all the
        // edges in the root graph.
        visualizeFrameEdges(outFile, frame.get());
        outFile << "}" << std::endl;
    }

    return 0;
}
