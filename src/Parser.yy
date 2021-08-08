%require "3.2"
%language "c++"
%define api.value.type variant
%define api.value.automove
%define api.token.constructor
%param { hadron::Parser* hadronParser }
%skeleton "lalr1.cc"

%token <size_t> EMPTY LITERAL PRIMITIVE PLUS MINUS ASTERISK ASSIGN LESSTHAN GREATERTHAN PIPE READWRITEVAR LEFTARROW
%token <size_t> BINOP KEYWORD OPENPAREN CLOSEPAREN OPENCURLY CLOSECURLY OPENSQUARE CLOSESQUARE COMMA SEMICOLON COLON
%token <size_t> CARET TILDE HASH GRAVE VAR ARG CONST CLASSVAR IDENTIFIER CLASSNAME DOT DOTDOT ELLIPSES CURRYARGUMENT

%type <std::unique_ptr<hadron::parse::Node>> root funcbody expr exprn exprseq classes classextensions
%type <std::unique_ptr<hadron::parse::ClassNode>> classdef
%type <std::unique_ptr<hadron::parse::ClassExtNode>> classextension
%type <std::unique_ptr<hadron::parse::VarListNode>> classvardecl classvardecls
%type <std::unique_ptr<hadron::parse::MethodNode>> methods methoddef
%type <std::unique_ptr<hadron::parse::BlockNode>> cmdlinecode methbody
%type <std::unique_ptr<hadron::parse::ReturnNode>> funretval

%type <std::optional<size_t>> superclass optname primitive

%{
#include "hadron/Parser.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
%}

%code{
yy::parser::symbol_type yylex(hadron::Parser* hadronParser);
}

%%
root    : classes
            { hadronParser->setRoot($classes); }
        | classextensions
            { hadronParser->setRoot($classextensions); }
        | cmdlinecode
            { hadronParser->setRoot($cmdlinecode); }
        ;

classes[target] :
                    {
                        $target = std::make_unique<hadron::parse::Node>(hadron::parse::NodeType::kEmpty, 0);
                    }
                | classes[build] classdef
                    {
                        $build->append($classdef);
                        $target = std::move($build);
                    }
                ;

classextensions[target] : classextension
                        | classextensions[build] classextension
                            {
                                $build->append($classextension);
                                $target = std::move($build);
                            }
                        ;

classdef    : CLASSNAME superclass OPENCURLY classvardecls methods CLOSECURLY
                {
                    auto classDef = std::make_unique<hadron::parse::ClassNode>($CLASSNAME);
                    classDef->superClassNameIndex = $superclass;
                    classDef->variables = std::move($classvardecls);
                    classDef->methods = std::move($methods);
                    $classdef = std::move(classDef);
                }
            | CLASSNAME OPENSQUARE optname CLOSESQUARE superclass OPENCURLY classvardecls methods CLOSECURLY
                {
                    auto classDef = std::make_unique<hadron::parse::ClassNode>($CLASSNAME);
                    classDef->superClassNameIndex = $superclass;
                    classDef->optionalNameIndex = $optname;
                    classDef->variables = std::move($classvardecls);
                    classDef->methods = std::move($methods);
                    $classdef = std::move(classDef);
                }
            ;

classextension  : PLUS CLASSNAME OPENCURLY methods CLOSECURLY
                    {
                        auto classExt = std::make_unique<hadron::parse::ClassExtNode>($CLASSNAME);
                        classExt->methods = std::move($methods);
                        $classextension = std::move(classExt);
                    }
                ;

optname : { $optname = std::nullopt; }
        | IDENTIFIER
            { $optname = $IDENTIFIER; }
        ;

superclass  : { $superclass = std::nullopt; }
            | COLON CLASSNAME
                { $superclass = $CLASSNAME; }

classvardecls[target]   : { $target = nullptr; }
                        | classvardecls[build] classvardecl
                            {
                                $build->append($classvardecl);
                                $target = std::move($build);
                            }
                        ;

classvardecl    : CLASSVAR rwslotdeflist SEMICOLON
                    {
                        auto varList = std::make_unique<hadron::parse::VarListNode>($CLASSVAR);
                        varList->definitions = std::move($rwslotdeflist);
                        $classvardecl = std::move(varList);
                    }
                | VAR rwslotdeflist SEMICOLON
                    {
                        auto varList = std::make_unique<hadron::parse::VarListNode>($VAR);
                        varList->definitions = std::move($rwslotdeflist);
                        $classvardecl = std::move(varList);
                    }
                | CONST constdeflist SEMICOLON
                    {
                        auto varList = std::make_unique<hadron::parse::VarListNode>($CONST);
                        varList->definitions = std::move($constdeflist);
                        $classvardecl = std::move(varList);
                    }
                ;

methods[target] : { $target = nullptr; }
                | methods[build] methoddef
                    {
                        $build->append($methoddef);
                        $target = std::move($build);
                    }
                ;

methoddef   : IDENTIFIER OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY
                {
                    auto method = std::make_unique<hadron::parse::MethodNode>($IDENTIFIER, false);
                    method->primitiveIndex = $primitive;
                    method->body = std::move($methbody);
                    $methoddef = std::move(method);
                }
            | ASTERISK IDENTIFIER OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY
                {
                    auto method = std::make_unique<hadron::parse::MethodNode>($IDENTIFIER, true);
                    method->primitiveIndex = $primitive;
                    method->body = std::move($methbody);
                    $methoddef = std::move(method);
                }
// TODO: binops
            ;

optsemi :
        | ';'
        ;

optcomma	:
			| ','
			;

optequal	:
			| '='
			;

funcbody    : funretval
                { $funcbody = std::move($funretval); }
            | exprseq funretval
                {
                    auto exprSeq = std::make_unique<hadron::parse::ExprSeqNode>(hadronParser->tokenIndex(),
                        std::move($exprseq));
                    exprSeq->expr->append($funretval);
                    $funcbody = std::move(exprSeq);
                }
            ;


cmdlinecode : funcbody
                {
                    auto block = std::make_unique<hadron::parse::BlockNode>(hadronParser->tokenIndex());
                    block->body = $funcbody;
                    $cmdlinecode = std::move(block);
                }
            ;

methbody    : retval
            | exprseq retval
                {
                    
                }

funretval   :
                { $funretval = std::make_unique<hadron::parse::ReturnNode>(hadronParser->tokenIndex()); }
            | CARET expr optsemi
                {
                    auto ret = std::make_unique<hadron::parse::ReturnNode>(hadronParser->tokenIndex());
                    ret->valueExpr = std::move($expr);
                    $funretval = std::move(ret);
                }
            ;

exprn[target]   : expr
                | exprn[build] ';' expr
                    {
                        $build->append(std::move($expr));
                        $target = std::move($build);
                    }
                ;

exprseq : exprn optsemi
            { $exprseq = std::move($exprn); }
        ;

expr[target]    : LITERAL
                    {
                        $target = std::make_unique<hadron::parse::LiteralNode>(hadronParser->tokenIndex(),
                            hadronParser->token().value);
                    }
                ;

%%

yy::parser::symbol_type yylex(hadron::Parser* hadronParser) {
    if (!hadronParser->next()) {
        return yy::parser::make_YYEOF();
    }
    switch (hadronParser->token().name) {
    case hadron::Token::Name::kEmpty:
        return yy::parser::make_EMPTY(hadronParser->tokenIndex());

    case hadron::Token::Name::kLiteral:
        return yy::parser::make_LITERAL(hadronParser->tokenIndex());

    case hadron::Token::Name::kPrimitive:
        return yy::parser::make_PRIMITIVE(hadronParser->tokenIndex());

    case hadron::Token::Name::kPlus:
        return yy::parser::make_PLUS(hadronParser->tokenIndex());

    case hadron::Token::Name::kMinus:
        return yy::parser::make_MINUS(hadronParser->tokenIndex());

    case hadron::Token::Name::kAsterisk:
        return yy::parser::make_ASTERISK(hadronParser->tokenIndex());

    case hadron::Token::Name::kAssign:
        return yy::parser::make_ASSIGN(hadronParser->tokenIndex());

    case hadron::Token::Name::kLessThan:
        return yy::parser::make_LESSTHAN(hadronParser->tokenIndex());

    case hadron::Token::Name::kGreaterThan:
        return yy::parser::make_GREATERTHAN(hadronParser->tokenIndex());

    case hadron::Token::Name::kPipe:
        return yy::parser::make_PIPE(hadronParser->tokenIndex());

    case hadron::Token::Name::kReadWriteVar:
        return yy::parser::make_READWRITEVAR(hadronParser->tokenIndex());

    case hadron::Token::Name::kLeftArrow:
        return yy::parser::make_LEFTARROW(hadronParser->tokenIndex());

    case hadron::Token::Name::kBinop:
        return yy::parser::make_BINOP(hadronParser->tokenIndex());

    case hadron::Token::Name::kKeyword:
        return yy::parser::make_KEYWORD(hadronParser->tokenIndex());

    case hadron::Token::Name::kOpenParen:
        return yy::parser::make_OPENPAREN(hadronParser->tokenIndex());

    case hadron::Token::Name::kCloseParen:
        return yy::parser::make_CLOSEPAREN(hadronParser->tokenIndex());

    case hadron::Token::Name::kOpenCurly:
        return yy::parser::make_OPENCURLY(hadronParser->tokenIndex());

    case hadron::Token::Name::kCloseCurly:
        return yy::parser::make_CLOSECURLY(hadronParser->tokenIndex());

    case hadron::Token::Name::kOpenSquare:
        return yy::parser::make_OPENSQUARE(hadronParser->tokenIndex());

    case hadron::Token::Name::kCloseSquare:
        return yy::parser::make_CLOSESQUARE(hadronParser->tokenIndex());

    case hadron::Token::Name::kComma:
        return yy::parser::make_COMMA(hadronParser->tokenIndex());

    case hadron::Token::Name::kSemicolon:
        return yy::parser::make_SEMICOLON(hadronParser->tokenIndex());

    case hadron::Token::Name::kColon:
        return yy::parser::make_COLON(hadronParser->tokenIndex());

    case hadron::Token::Name::kCaret:
        return yy::parser::make_CARET(hadronParser->tokenIndex());

    case hadron::Token::Name::kTilde:
        return yy::parser::make_TILDE(hadronParser->tokenIndex());

    case hadron::Token::Name::kHash:
        return yy::parser::make_HASH(hadronParser->tokenIndex());

    case hadron::Token::Name::kGrave:
        return yy::parser::make_GRAVE(hadronParser->tokenIndex());

    case hadron::Token::Name::kVar:
        return yy::parser::make_VAR(hadronParser->tokenIndex());

    case hadron::Token::Name::kArg:
        return yy::parser::make_ARG(hadronParser->tokenIndex());

    case hadron::Token::Name::kConst:
        return yy::parser::make_CONST(hadronParser->tokenIndex());

    case hadron::Token::Name::kClassVar:
        return yy::parser::make_CLASSVAR(hadronParser->tokenIndex());

    case hadron::Token::Name::kIdentifier:
        return yy::parser::make_IDENTIFIER(hadronParser->tokenIndex());

    case hadron::Token::Name::kClassName:
        return yy::parser::make_CLASSNAME(hadronParser->tokenIndex());

    case hadron::Token::Name::kDot:
        return yy::parser::make_DOT(hadronParser->tokenIndex());

    case hadron::Token::Name::kDotDot:
        return yy::parser::make_DOTDOT(hadronParser->tokenIndex());

    case hadron::Token::Name::kEllipses:
        return yy::parser::make_ELLIPSES(hadronParser->tokenIndex());

    case hadron::Token::Name::kCurryArgument:
        return yy::parser::make_CURRYARGUMENT(hadronParser->tokenIndex());
    }

    return yy::parser::make_YYEOF();
}

void yy::parser::error(const std::string& /* errorString */) {
}

namespace hadron {

Parser::Parser(Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter):
    m_lexer(lexer),
    m_tokenIndex(0),
    m_token(Token()),
    m_errorReporter(errorReporter) { }

Parser::Parser(std::string_view code):
    m_ownLexer(std::make_unique<Lexer>(code)),
    m_lexer(m_ownLexer.get()),
    m_tokenIndex(0),
    m_token(Token()),
    m_errorReporter(m_ownLexer->errorReporter()) { }

Parser::~Parser() {}

bool Parser::parse() {
    if (m_ownLexer) {
        if (!m_lexer->lex()) {
            return false;
        }
    }

    if (m_lexer->tokens().size() > 0) {
        m_token = m_lexer->tokens()[0];
    } else {
        m_token = Token(Token::Name::kEmpty, nullptr, 0);
    }

    m_root = nullptr;
    m_tokenIndex = 0;
    yy::parser parser(this);
    auto error = parser.parse();
    if (error != 0) {
        return false;
    }

    if (!m_root) return false;

//    while (m_errorReporter->errorCount() == 0 && m_token.name != Token::Name::kEmpty) {
//        m_root->append(parseRoot());
//    }

    return (m_root != nullptr && m_errorReporter->errorCount() == 0);
}

bool Parser::next() {
    ++m_tokenIndex;
    if (m_tokenIndex < m_lexer->tokens().size()) {
        m_token = m_lexer->tokens()[m_tokenIndex];
        return true;
    }
    m_token = Token(Token::Name::kEmpty, nullptr, 0);
    return false;
}

} // namespace hadron