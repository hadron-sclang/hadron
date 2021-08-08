%require "3.2"
%language "c++"
%define api.value.type variant
%define api.token.constructor
%param { hadron::Parser* hadronParser }
%skeleton "lalr1.cc"

%token <size_t> EMPTY LITERAL PRIMITIVE PLUS MINUS ASTERISK ASSIGN LESSTHAN GREATERTHAN PIPE READWRITEVAR LEFTARROW
%token <size_t> BINOP KEYWORD OPENPAREN CLOSEPAREN OPENCURLY CLOSECURLY OPENSQUARE CLOSESQUARE COMMA SEMICOLON COLON
%token <size_t> CARET TILDE HASH GRAVE VAR ARG CONST CLASSVAR IDENTIFIER CLASSNAME DOT DOTDOT ELLIPSES CURRYARGUMENT

%type <std::unique_ptr<hadron::parse::ArgListNode>> argdecls
%type <std::unique_ptr<hadron::parse::BlockNode>> cmdlinecode
%type <std::unique_ptr<hadron::parse::ClassExtNode>> classextension
%type <std::unique_ptr<hadron::parse::ClassNode>> classdef
%type <std::unique_ptr<hadron::parse::LiteralNode>> literal
%type <std::unique_ptr<hadron::parse::MethodNode>> methods methoddef
%type <std::unique_ptr<hadron::parse::Node>> root funcbody expr exprn exprseq classes classextensions methbody
%type <std::unique_ptr<hadron::parse::ReturnNode>> funretval retval
%type <std::unique_ptr<hadron::parse::VarDefNode>> rwslotdeflist rwslotdef slotdef vardef constdef constdeflist
%type <std::unique_ptr<hadron::parse::VarDefNode>> vardeflist slotdeflist
%type <std::unique_ptr<hadron::parse::VarListNode>> classvardecl classvardecls funcvardecls funcvardecl

%type <std::optional<size_t>> superclass optname primitive
%type <std::pair<bool, bool>> rwspec
%type <bool> rspec

%{
#include "hadron/Parser.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
%}

%code{
// see linkNextNode() in LSC
std::unique_ptr<hadron::parse::Node> append(std::unique_ptr<hadron::parse::Node> head,
    std::unique_ptr<hadron::parse::Node> tail);
yy::parser::symbol_type yylex(hadron::Parser* hadronParser);
}

%%
root    : classes
            { hadronParser->setRoot(std::move($classes)); }
        | classextensions
            { hadronParser->setRoot(std::move($classextensions)); }
        | cmdlinecode
            { hadronParser->setRoot(std::move($cmdlinecode)); }
        ;

classes[target] :
                    { $target = std::make_unique<hadron::parse::Node>(hadron::parse::NodeType::kEmpty, 0); }
                | classes[build] classdef
                    { $target = append(std::move($build), std::move($classdef)); }
                ;

classextensions[target] : classextension
                            { $target = std::move($classextension); }
                        | classextensions[build] classextension
                            {   $target = append(std::move($build), std::move($classextension)); }
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
                            { $target = append(std::move($build), std::move($classvardecl)); }
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
                    { $target = append(std::move($build), std::move($methoddef)); }
                ;

methoddef   : IDENTIFIER OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY
                {
                    auto method = std::make_unique<hadron::parse::MethodNode>($IDENTIFIER, false);
                    method->primitiveIndex = $primitive;
                    method->body = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                    method->body->arguments = std::move($argdecls);
                    method->body->variables = std::move($funcvardecls);
                    method->body->body = std::move($methbody);
                    $methoddef = std::move(method);
                }
            | ASTERISK IDENTIFIER OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY
                {
                    auto method = std::make_unique<hadron::parse::MethodNode>($IDENTIFIER, true);
                    method->primitiveIndex = $primitive;
                    method->body = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                    method->body->arguments = std::move($argdecls);
                    method->body->variables = std::move($funcvardecls);
                    method->body->body = std::move($methbody);
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
                    exprSeq->append(std::move($funretval));
                    $funcbody = std::move(exprSeq);
                }
            ;


cmdlinecode : funcbody
                {
                    auto block = std::make_unique<hadron::parse::BlockNode>(hadronParser->tokenIndex());
                    block->body = std::move($funcbody);
                    $cmdlinecode = std::move(block);
                }
            ;

// TODO: how is this different from funcbody et al? Difference in LSC grammar between BlockReturn and Return
methbody    : retval
                { $methbody = std::move($retval); }
            | exprseq retval
                {
                    $exprseq->append(std::move($retval));
                    $methbody = std::move($exprseq);
                }
            ;

primitive   : { $primitive = std::nullopt; }
            | PRIMITIVE optsemi
                {
                    { $primitive = $PRIMITIVE; }
                }
            ;

retval  : { $retval = std::make_unique<hadron::parse::ReturnNode>(hadronParser->tokenIndex()); }
        | CARET expr optsemi
            {
                auto ret = std::make_unique<hadron::parse::ReturnNode>(hadronParser->tokenIndex());
                ret->valueExpr = std::move($expr);
                $retval = std::move(ret);
            }
        ;

funretval   : { $funretval = std::make_unique<hadron::parse::ReturnNode>(hadronParser->tokenIndex()); }
            | CARET expr optsemi
                {
                    auto ret = std::make_unique<hadron::parse::ReturnNode>(hadronParser->tokenIndex());
                    ret->valueExpr = std::move($expr);
                    $funretval = std::move(ret);
                }
            ;

exprn[target]   : expr
                    { $target = std::move($expr); }
                | exprn[build] ';' expr
                    {   $target = append(std::move($build), std::move($expr)); }
                ;

exprseq : exprn optsemi
            { $exprseq = std::move($exprn); }
        ;

expr[target]    : literal
                    { $target = std::move($literal); }
                ;

funcvardecls[target]    : { $target = nullptr; }
                        | funcvardecls[build] funcvardecl
                            { $target = append(std::move($build), std::move($funcvardecl)); }
                        ;

funcvardecl : VAR vardeflist SEMICOLON
                {
                    auto varList = std::make_unique<hadron::parse::VarListNode>($VAR);
                    varList->definitions = std::move($vardeflist);
                    $funcvardecl = std::move(varList);
                }
            ;

// TODO: vargargs
argdecls    : { $argdecls = nullptr; }
            | ARG vardeflist SEMICOLON
                {
                    auto argList = std::make_unique<hadron::parse::ArgListNode>($ARG);
                    argList->varList = std::make_unique<hadron::parse::VarListNode>($ARG);
                    argList->varList->definitions = std::move($vardeflist);
                    $argdecls = std::move(argList);
                }
            | PIPE[open] slotdeflist PIPE
                {
                    auto argList = std::make_unique<hadron::parse::ArgListNode>($open);
                    argList->varList = std::make_unique<hadron::parse::VarListNode>($open);
                    argList->varList->definitions = std::move($slotdeflist);
                    $argdecls = std::move(argList);
                }
            ;


constdeflist[target]    : constdef
                            { $target = std::move($constdef); }
                        | constdeflist[build] optcomma constdef
                            { $target = append(std::move($build), std::move($constdef)); }
                        ;

constdef    : rspec IDENTIFIER ASSIGN literal
                {
                    auto constDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                    constDef->hasReadAccessor = $rspec;
                    constDef->initialValue = std::move($literal);
                    $constdef = std::move(constDef);
                }
            ;

slotdeflist[target] : slotdef
                        { $target = std::move($slotdef); }
                    | slotdeflist[build] optcomma slotdef
                        { $target = append(std::move($build), std::move($slotdef)); }
                    ;

slotdef : IDENTIFIER
            { $slotdef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER); }
        | IDENTIFIER optequal literal
            {
                auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                varDef->initialValue = std::move($literal);
                $slotdef = std::move(varDef);
            }
        ;

vardeflist[target]  : vardef
                        { $target = std::move($vardef); }
                    | vardeflist[build] COMMA vardef
                        { $target = append(std::move($build), std::move($vardef)); }
                    ;

vardef  : IDENTIFIER
            { $vardef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER); }
        | IDENTIFIER ASSIGN expr
            {
                auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                varDef->initialValue = std::move($expr);
                $vardef = std::move(varDef);
            }
        ;

rwslotdeflist[target]   : rwslotdef
                            { $target = std::move($rwslotdef); }
                        | rwslotdeflist[build] COMMA rwslotdef
                            { $target = append(std::move($build), std::move($rwslotdef)); }
                        ;

rwslotdef   : rwspec IDENTIFIER
                {
                    auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                    varDef->hasReadAccessor = $rwspec.first;
                    varDef->hasWriteAccessor = $rwspec.second;
                    $rwslotdef = std::move(varDef);
                }
            | rwspec IDENTIFIER ASSIGN literal
                {
                    auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                    varDef->hasReadAccessor = $rwspec.first;
                    varDef->hasWriteAccessor = $rwspec.second;
                    varDef->initialValue = std::move($literal);
                    $rwslotdef = std::move(varDef);
                }
            ;

rwspec  : { $rwspec = std::make_pair(false, false); }
        | LESSTHAN
            { $rwspec = std::make_pair(true, false); }
        | GREATERTHAN
            { $rwspec = std::make_pair(false, true); }
        | READWRITEVAR
            { $rwspec = std::make_pair(true, true); }
        ;

rspec   : { $rspec = false; }
        | LESSTHAN
            { $rspec = true; }
        ;

literal : LITERAL
            {
                $literal = std::make_unique<hadron::parse::LiteralNode>($LITERAL, hadronParser->token().value);
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

std::unique_ptr<hadron::parse::Node> append(std::unique_ptr<hadron::parse::Node> head,
        std::unique_ptr<hadron::parse::Node> tail) {
    if (!head) {
        return tail;
    }
    if (tail) {
        head->append(std::move(tail));
    }
    return head;
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