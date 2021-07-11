// vistool generates dot files, suitable for consumption with graphviz, of different outputs of the Hadron compiler
// processess. It currently can generate parse tree and Control Flow Graph (CFG) block tree visualizations.
#include "FileSystem.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Hash.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Literal.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SyntaxAnalyzer.hpp"
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
DEFINE_bool(syntaxTree, false, "print the syntax tree");
DEFINE_bool(optimizeTree, false, "optimize the syntax tree before printing");

std::string nullOrNo(const hadron::parse::Node* node) {
    if (node) return "";
    return "<i>null</i>";
}

std::string trueFalse(bool v) {
    if (v) return "true";
    return "false";
}

std::string printType(const hadron::Type type) {
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

        case hadron::Type::kSlot:
            return "(slot)";
    }

    return "(unknown type!)";
}

std::string printLiteral(const hadron::Literal& literal) {
    std::string type = printType(literal.type());
    switch (literal.type()) {
    case hadron::Type::kInteger:
        return fmt::format("(int) {}", literal.asInteger());

    case hadron::Type::kFloat:
        return fmt::format("(float) {}", literal.asFloat());

    case hadron::Type::kBoolean:
        return fmt::format("(bool) {}", trueFalse(literal.asBoolean()));

    default:
        break;
    }

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

void visualizeAST(std::ofstream& outFile, int& serial, const hadron::ast::AST* ast,
        std::vector<std::set<std::string>>& regs, std::map<hadron::Hash, std::string>& symbols) {
    int astSerial = serial;
    ++serial;

    switch (ast->astType) {
    case hadron::ast::ASTType::kCalculate: {
        const auto calc = reinterpret_cast<const hadron::ast::CalculateAST*>(ast);
        outFile << fmt::format("    ast_{} [label=\"{} {}\"]\n", astSerial, printHash(calc->selector),
            printType(ast->valueType));
        if (calc->left) {
            outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
            visualizeAST(outFile, serial, calc->left.get(), regs, symbols);
        }
        if (calc->right) {
            outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
            visualizeAST(outFile, serial, calc->right.get(), regs, symbols);
        }
    } break;

    case hadron::ast::ASTType::kAssign: {
        const auto assign = reinterpret_cast<const hadron::ast::AssignAST*>(ast);
        outFile << fmt::format("    ast_{} [label=<&#8592; {}>]\n", astSerial, printType(ast->valueType));
        if (assign->target) {
            outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
            visualizeAST(outFile, serial, assign->target.get(), regs, symbols);
        }
        if (assign->value) {
            outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
            visualizeAST(outFile, serial, assign->value.get(), regs, symbols);
        }
    } break;

    case hadron::ast::ASTType::kBlock: {
        const auto block = reinterpret_cast<const hadron::ast::BlockAST*>(ast);
        outFile << fmt::format("    ast_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>Block {}</b></td></tr>", astSerial, printType(ast->valueType));
        if (block->arguments.size()) {
            outFile << "<tr><td><b>arg:</b>";
            for (auto pair : block->arguments) {
                outFile << " " << pair.second.name;
                symbols.emplace(std::make_pair(pair.first, pair.second.name));
            }
            outFile << "</td></tr>";
        }
        if (block->variables.size()) {
            outFile << "<tr><td><b>var:</b>";
            for (auto pair : block->variables) {
                symbols.emplace(std::make_pair(pair.first, pair.second.name));
                outFile << " " << pair.second.name;
            }
            outFile << "</td></tr>";
        }
        outFile << "</table>>]\n";
        regs.resize(block->numberOfVirtualRegisters);
        for (const auto& expr : block->statements) {
            if (!expr) continue;
            outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
            visualizeAST(outFile, serial, expr.get(), regs, symbols);
        }
    } break;

    case hadron::ast::ASTType::kInlineBlock: {
        const auto inlineBlock = reinterpret_cast<const hadron::ast::InlineBlockAST*>(ast);
        outFile << fmt::format("    ast_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">"
            "<tr><td bgcolor=\"lightGray\"><b>InlineBlock {}</b></td></tr>", astSerial, printType(ast->valueType));
        outFile << "</table>>]\n";
        for (const auto& expr : inlineBlock->statements) {
            if (!expr) continue;
            outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
            visualizeAST(outFile, serial, expr.get(), regs, symbols);
        }
    } break;

    case hadron::ast::ASTType::kValue: {
        const auto value = reinterpret_cast<const hadron::ast::ValueAST*>(ast);
        outFile << fmt::format("    ast_{} [label=<{} <i>{}</i>", astSerial, printType(value->valueType),
            findSymbol(value->nameHash, symbols));
        if (value->registerNumber >= 0) {
            outFile << fmt::format(" <font face=\"monospace\">r<sub>{}</sub></font>>]\n", value->registerNumber);
        } else {
            outFile << ">]" << std::endl;
        }
    } break;

    case hadron::ast::ASTType::kConstant: {
        const auto constant = reinterpret_cast<const hadron::ast::ConstantAST*>(ast);
        outFile << fmt::format("    ast_{} [label=<<b>{}</b>>]\n", astSerial, printLiteral(constant->value));
    } break;

    case hadron::ast::ASTType::kWhile: {
        const auto whileAST = reinterpret_cast<const hadron::ast::WhileAST*>(ast);
        outFile << fmt::format("    ast_{} [label=\"while {}\"]\n", astSerial, printType(ast->valueType));
        if (whileAST->condition) {
            outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
            visualizeAST(outFile, serial, whileAST->condition.get(), regs, symbols);
        }
        if (whileAST->action) {
            outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
            visualizeAST(outFile, serial, whileAST->action.get(), regs, symbols);
        }
    } break;

    case hadron::ast::ASTType::kDispatch: {
        const auto dispatch = reinterpret_cast<const hadron::ast::DispatchAST*>(ast);
        outFile << fmt::format("    ast_{} [label=\"{} {}\"]\n", astSerial, dispatch->selector,
            printType(ast->valueType));
        for (const auto& expr : dispatch->arguments) {
            outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
            visualizeAST(outFile, serial, expr.get(), regs, symbols);
        }
    } break;

    case hadron::ast::ASTType::kLoadAddress: {
        const auto loadAddress = reinterpret_cast<const hadron::ast::LoadAddressAST*>(ast);
        outFile << fmt::format("    ast_{} [label=\"&\"]\n", astSerial);
        outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
        visualizeAST(outFile, serial, loadAddress->address.get(), regs, symbols);
    } break;

    case hadron::ast::ASTType::kLoad: {

    } break;

    case hadron::ast::ASTType::kStore: {
        const auto store = reinterpret_cast<const hadron::ast::StoreAST*>(ast);
        outFile << fmt::format("    ast_{} [label=\"store\"]\n", astSerial);
        if (store->address) {
            outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
            visualizeAST(outFile, serial, store->address.get(), regs, symbols);
        }
        if (store->offset) {
            outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
            visualizeAST(outFile, serial, store->offset.get(), regs, symbols);
        }
        if (store->value) {
            outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
            visualizeAST(outFile, serial, store->value.get(), regs, symbols);
        }
    } break;

    case hadron::ast::ASTType::kLoadFromSlot: {

    } break;

    case hadron::ast::ASTType::kSaveToSlot: {
        const auto save = reinterpret_cast<const hadron::ast::SaveToSlotAST*>(ast);
        outFile << fmt::format("    ast_{} [shape=house label=\"slot store\"]\n", astSerial);
        outFile << fmt::format("    ast_{} -> ast_{}\n", astSerial, serial);
        visualizeAST(outFile, serial, save->value.get(), regs, symbols);
    } break;

    case hadron::ast::ASTType::kAliasRegister: {
        const auto alias = reinterpret_cast<const hadron::ast::AliasRegisterAST*>(ast);
        outFile << fmt::format("    ast_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" "
            "cellspacing=\"0\">\n", astSerial);
        outFile << fmt::format("        <tr><td><b>alias</b> <i>{}</i> &#8658; "
            "<font face=\"monospace\">r<sub>{}</sub></font></td></tr>\n", findSymbol(alias->nameHash, symbols),
            alias->registerNumber);
        regs[alias->registerNumber].insert(findSymbol(alias->nameHash, symbols));
        for (size_t i = 0; i < regs.size(); ++i) {
            outFile << fmt::format("        <tr><td align=\"left\"><font face=\"monospace\">r<sub>{}</sub></font>:", i);
            for (auto& name : regs[i]) {
                outFile << " <i>" << name << "</i>";
            }
            outFile << "</td></tr>" << std::endl;
        }
        outFile << "        </table>>]" << std::endl;
    } break;

    case hadron::ast::ASTType::kUnaliasRegister: {
        const auto unalias = reinterpret_cast<const hadron::ast::UnaliasRegisterAST*>(ast);
        outFile << fmt::format("    ast_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" "
            "cellspacing=\"0\">\n", astSerial);
        outFile << fmt::format("        <tr><td><b>unalias</b> <i>{}</i> from "
            "<font face=\"monospace\">r<sub>{}</sub></font></td></tr>\n", findSymbol(unalias->nameHash, symbols),
            unalias->registerNumber);
        regs[unalias->registerNumber].erase(findSymbol(unalias->nameHash, symbols));
        for (size_t i = 0; i < regs.size(); ++i) {
            outFile << fmt::format("        <tr><td align=\"left\"><font face=\"monospace\">r<sub>{}</sub></font>:", i);
            for (auto& name : regs[i]) {
                outFile << " <i>" << name << "</i>";
            }
            outFile << "</td></tr>" << std::endl;
        }
        outFile << "        </table>>]" << std::endl;
    } break;

    case hadron::ast::ASTType::kClass: {
        const auto classAST = reinterpret_cast<const hadron::ast::ClassAST*>(ast);
        outFile << fmt::format("    ast_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" "
            "cellspacing=\"0\">\n", astSerial);
        outFile <<  fmt::format("       <tr><td bgcolor=\"lightGray\"><b>Class {}</b></td></tr>\n",
            printType(ast->valueType));
        if (classAST->variables.size()) {
            outFile << fmt::format("        <tr><td><font face=\"monospace\">{}</font></td></tr>\n", classAST->name);
            outFile << "        <tr><td><font face=\"monospace\">var:</font>";
            for (auto pair : classAST->variables) {
                outFile << " " << pair.second.name;
            }
            outFile << "</td></tr>\n";
        }
        if (classAST->classVariables.size()) {
            outFile << fmt::format("        <tr><td><font face=\"monospace\">{}</font></td></tr>\n", classAST->name);
            outFile << "        <tr><td><font face=\"monospace\">classvar:</font>";
            for (auto pair : classAST->classVariables) {
                outFile << " " << pair.second.name;
            }
            outFile << "</td></tr>\n";
        }
        if (classAST->constants.size()) {
            outFile << fmt::format("        <tr><td><font face=\"monospace\">{}</font></td></tr>\n", classAST->name);
            outFile << "        <tr><td><font face=\"monospace\">const:</font>";
            for (auto pair : classAST->constants) {
                std::string constName = classAST->names.find(pair.first)->second;
                outFile << constName << "=" << printLiteral(pair.second);
            }
            outFile << "</td></tr>\n";
        }
    } break;
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
            outFile << fmt::format("        line_{} [shape=plain label=<<table border=\"0\" cellborder=\"1\" "
                "cellspacing=\"0\"><tr><td><font point-size=\"24\">line {}:</font></td>", currentLine, currentLine);
            size_t tokenLine = errorReporter->getLineNumber(parser.lexer()->tokens()[tokenIndex].range.data());
            int tokenLineCount = 0;
            while (tokenLine == currentLine) {
                outFile << fmt::format("<td port=\"token_{}\"><font face=\"monospace\" "
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
    } else if (FLAGS_syntaxTree) {
        hadron::SyntaxAnalyzer syntaxAnalyzer(&parser, errorReporter);
        if (!syntaxAnalyzer.buildAST()) {
            spdlog::error("Failed to build AST from parse tree in file {}", filePath.string());
            return -1;
        }

        if (FLAGS_optimizeTree) {
//            syntaxAnalyzer.toThreeAddressForm();
//            syntaxAnalyzer.assignVirtualRegisters();
        }

        outFile << fmt::format("// syntax tree visualization of {}\n", FLAGS_inputFile);
        outFile << "digraph HadronSyntaxTree {" << std::endl;
        int serial = 0;
        std::vector<std::set<std::string>> regs;
        std::map<hadron::Hash, std::string> symbols;
        visualizeAST(outFile, serial, syntaxAnalyzer.ast(), regs, symbols);
        outFile << "}" << std::endl;
    }

    return 0;
}
