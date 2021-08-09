%require "3.2"
%language "c++"
%define api.value.type variant
%define api.token.constructor
%define parse.trace
%define parse.error verbose
%locations
%define api.location.type { std::string_view }
%param { hadron::Parser* hadronParser }
%skeleton "lalr1.cc"

%token END 0 "end of file"
%token <size_t> LITERAL FLOAT INTEGER PRIMITIVE PLUS MINUS ASTERISK ASSIGN LESSTHAN GREATERTHAN PIPE READWRITEVAR
%token <size_t> LEFTARROW BINOP KEYWORD OPENPAREN CLOSEPAREN OPENCURLY CLOSECURLY OPENSQUARE CLOSESQUARE COMMA
%token <size_t> SEMICOLON COLON CARET TILDE HASH GRAVE VAR ARG CONST CLASSVAR IDENTIFIER CLASSNAME DOT DOTDOT ELLIPSES
%token <size_t> CURRYARGUMENT

%type <std::unique_ptr<hadron::parse::ArgListNode>> argdecls
%type <std::unique_ptr<hadron::parse::BlockNode>> cmdlinecode block
%type <std::unique_ptr<hadron::parse::ClassExtNode>> classextension classextensions
%type <std::unique_ptr<hadron::parse::ClassNode>> classdef classes
%type <std::unique_ptr<hadron::parse::LiteralNode>> literal
%type <std::unique_ptr<hadron::parse::MethodNode>> methods methoddef
%type <std::unique_ptr<hadron::parse::Node>> root expr exprn expr1
%type <std::unique_ptr<hadron::parse::ReturnNode>> funretval retval
%type <std::unique_ptr<hadron::parse::VarDefNode>> rwslotdeflist rwslotdef slotdef vardef constdef constdeflist
%type <std::unique_ptr<hadron::parse::VarDefNode>> vardeflist slotdeflist vardeflist0 slotdeflist0
%type <std::unique_ptr<hadron::parse::VarListNode>> classvardecl classvardecls funcvardecls funcvardecl funcvardecls1
%type <std::unique_ptr<hadron::parse::ExprSeqNode>> exprseq methbody funcbody

%type <std::optional<size_t>> superclass optname primitive
%type <std::pair<bool, bool>> rwspec
%type <bool> rspec
%type <std::pair<size_t, int32_t>> integer
%type <std::pair<size_t, double>> float
%type <size_t> binop

%start root

%{
#include "hadron/Parser.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"

#include "spdlog/spdlog.h"

#include <string_view>
%}

%code{
// see linkNextNode() in LSC
template <typename T>
T append(T head, T tail) {
   if (!head) {
        return tail;
    }
    if (tail) {
        head->append(std::move(tail));
    }
    return head;
}

#define YYLLOC_DEFAULT(current, rhs, n)                                                                                \
    {                                                                                                                  \
      if (n) {                                                                                                         \
          current = std::string_view(YYRHSLOC(rhs, 1).data(), YYRHSLOC(rhs, n).data() - YYRHSLOC(rhs, 1).data());      \
      } else {                                                                                                         \
          current = std::string_view();                                                                                \
      }                                                                                                                \
    }

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

classes[target] : /* empty */
                    { $target = nullptr;}
                | classes[build] classdef
                    { $target = append(std::move($build), std::move($classdef)); }
                ;

classextensions[target] : classextension
                            { $target = std::move($classextension); }
                        | classextensions[build] classextension
                            { $target = append(std::move($build), std::move($classextension)); }
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

optname : /* empty */
            { $optname = std::nullopt; }
        | IDENTIFIER
            { $optname = $IDENTIFIER; }
        ;

superclass  : /* empty */
                { $superclass = std::nullopt; }
            | COLON CLASSNAME
                { $superclass = $CLASSNAME; }

classvardecls[target]   : /* empty */
                            { $target = nullptr; }
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

methods[target] : /* empty */
                    { $target = nullptr; }
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
            | binop OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY
                {
                    auto method = std::make_unique<hadron::parse::MethodNode>($binop, false);
                    method->primitiveIndex = $primitive;
                    method->body = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                    method->body->arguments = std::move($argdecls);
                    method->body->variables = std::move($funcvardecls);
                    method->body->body = std::move($methbody);
                    $methoddef = std::move(method);
                }
            | ASTERISK binop OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY
                {
                    auto method = std::make_unique<hadron::parse::MethodNode>($binop, true);
                    method->primitiveIndex = $primitive;
                    method->body = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                    method->body->arguments = std::move($argdecls);
                    method->body->variables = std::move($funcvardecls);
                    method->body->body = std::move($methbody);
                    $methoddef = std::move(method);
                }
            ;

optsemi : /* empty */
        | SEMICOLON
        ;

optcomma	: /* empty */
			| COMMA
			;

optequal	: /* empty */
			| ASSIGN
			;

funcbody    : CARET expr optsemi
                {
                    auto ret = std::make_unique<hadron::parse::ReturnNode>($CARET);
                    ret->valueExpr = std::move($expr);
                    auto exprseq = std::make_unique<hadron::parse::ExprSeqNode>($CARET, std::move(ret));
                    $funcbody = std::move(exprseq);
                }
            | exprseq funretval
                {
                    if ($funretval) {
                        $exprseq->expr->append(std::move($funretval));
                    }
                    $funcbody = std::move($exprseq);
                }
            ;


cmdlinecode : OPENPAREN funcvardecls1 methbody CLOSEPAREN
                {
                    auto block = std::make_unique<hadron::parse::BlockNode>($OPENPAREN);
                    block->variables = std::move($funcvardecls1);
                    block->body = std::move($methbody);
                    $cmdlinecode = std::move(block);
                }
            | funcvardecls1 methbody
                {
                    auto block = std::make_unique<hadron::parse::BlockNode>($funcvardecls1->tokenIndex);
                    block->variables = std::move($funcvardecls1);
                    block->body = std::move($methbody);
                    $cmdlinecode = std::move(block);
                }
            | funcbody
                {
                    auto block = std::make_unique<hadron::parse::BlockNode>($funcbody->tokenIndex);
                    block->body = std::move($funcbody);
                    $cmdlinecode = std::move(block);
                }
            ;

// methbody can be optional, funcbody requires non-empty. TODO: rename for clarity.
methbody    : retval
                {
                    if ($retval) {
                        auto exprSeq = std::make_unique<hadron::parse::ExprSeqNode>($retval->tokenIndex,
                            std::move($retval));
                        $methbody = std::move(exprSeq);
                    } else {
                        $methbody = nullptr;
                    }
                }
            | exprseq retval
                {
                    if ($retval) {
                        $exprseq->expr->append(std::move($retval));
                    }
                    $methbody = std::move($exprseq);
                }
            ;

primitive   : /* empty */
                { $primitive = std::nullopt; }
            | PRIMITIVE optsemi
                { $primitive = $PRIMITIVE; }
            ;

retval  : /* empty */
            { $retval = nullptr; }
        | CARET expr optsemi
            {
                auto ret = std::make_unique<hadron::parse::ReturnNode>($CARET);
                ret->valueExpr = std::move($expr);
                $retval = std::move(ret);
            }
        ;

funretval   : /* empty */
                { $funretval = nullptr; }
            | CARET expr optsemi
                {
                    auto ret = std::make_unique<hadron::parse::ReturnNode>($CARET);
                    ret->valueExpr = std::move($expr);
                    $funretval = std::move(ret);
                }
            ;

exprn[target]   : expr
                    { $target = std::move($expr); }
                | exprn[build] SEMICOLON expr
                    {   $target = append(std::move($build), std::move($expr)); }
                ;

exprseq : exprn optsemi
            { $exprseq = std::make_unique<hadron::parse::ExprSeqNode>($exprn->tokenIndex, std::move($exprn)); }
        ;

expr1[target]   : literal
                    { $target = std::move($literal); }
                | IDENTIFIER
                    { $target = std::make_unique<hadron::parse::NameNode>($IDENTIFIER); }
                ;

expr[target]    : expr1
                    { $target = std::move($expr1); }
                ;

block   : OPENCURLY argdecls funcvardecls methbody CLOSECURLY
            {
                auto block = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                block->arguments = std::move($argdecls);
                block->variables = std::move($funcvardecls);
                block->body = std::move($methbody);
                $block = std::move(block);
            }
        ;

funcvardecls[target]    : /* empty */
                            { $target = nullptr; }
                        | funcvardecls[build] funcvardecl
                            { $target = append(std::move($build), std::move($funcvardecl)); }
                        ;

funcvardecls1[target]   : funcvardecl { $target = std::move($funcvardecl); }
                        | funcvardecls1[build] funcvardecl
                            { $target = append(std::move($build), std::move($funcvardecl)); }
                        ;

funcvardecl : VAR vardeflist SEMICOLON
                {
                    auto varList = std::make_unique<hadron::parse::VarListNode>($VAR);
                    varList->definitions = std::move($vardeflist);
                    $funcvardecl = std::move(varList);
                }
            ;

argdecls    : /* empty */
                { $argdecls = nullptr; }
            | ARG vardeflist SEMICOLON
                {
                    auto argList = std::make_unique<hadron::parse::ArgListNode>($ARG);
                    argList->varList = std::make_unique<hadron::parse::VarListNode>($ARG);
                    argList->varList->definitions = std::move($vardeflist);
                    $argdecls = std::move(argList);
                }
            | ARG vardeflist0 ELLIPSES IDENTIFIER SEMICOLON
                {
                    auto argList = std::make_unique<hadron::parse::ArgListNode>($ARG);
                    argList->varList = std::make_unique<hadron::parse::VarListNode>($ARG);
                    argList->varList->definitions = std::move($vardeflist0);
                    argList->varArgsNameIndex = $IDENTIFIER;
                    $argdecls = std::move(argList);
                }
            | PIPE[open] slotdeflist PIPE
                {
                    auto argList = std::make_unique<hadron::parse::ArgListNode>($open);
                    argList->varList = std::make_unique<hadron::parse::VarListNode>($open);
                    argList->varList->definitions = std::move($slotdeflist);
                    $argdecls = std::move(argList);
                }
            | PIPE[open] slotdeflist0 ELLIPSES IDENTIFIER PIPE
                {
                    auto argList = std::make_unique<hadron::parse::ArgListNode>($open);
                    argList->varList = std::make_unique<hadron::parse::VarListNode>($open);
                    argList->varList->definitions = std::move($slotdeflist0);
                    argList->varArgsNameIndex = $IDENTIFIER;
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

slotdeflist0    : /* empty */
                    { $slotdeflist0 = nullptr; }
                | slotdeflist
                    { $slotdeflist0 = std::move($slotdeflist); }
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

vardeflist0 : /* empty */
                { $vardeflist0 = nullptr; }
            | vardeflist
                { $vardeflist0 = std::move($vardeflist); }
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
        | IDENTIFIER OPENPAREN exprseq CLOSEPAREN
            {
                auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                varDef->initialValue = std::move($exprseq);
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

rwspec  : /* empty */ { $rwspec = std::make_pair(false, false); }
        | LESSTHAN { $rwspec = std::make_pair(true, false); }
        | GREATERTHAN { $rwspec = std::make_pair(false, true); }
        | READWRITEVAR { $rwspec = std::make_pair(true, true); }
        ;

rspec   : /* empty */ { $rspec = false; }
        | LESSTHAN { $rspec = true; }
        ;

literal : LITERAL
            { $literal = std::make_unique<hadron::parse::LiteralNode>($LITERAL, hadronParser->token($LITERAL).value); }
        | integer
            {
                $literal = std::make_unique<hadron::parse::LiteralNode>($integer.first,
                    hadron::Slot(hadron::Type::kInteger, hadron::Slot::Value($integer.second)));
            }
        | float
            {
                $literal = std::make_unique<hadron::parse::LiteralNode>($float.first,
                    hadron::Slot(hadron::Type::kFloat, hadron::Slot::Value($float.second)));
            }
        | block
            {
                auto literal = std::make_unique<hadron::parse::LiteralNode>($block->tokenIndex, hadron::Slot());
                literal->blockLiteral = std::move($block);
                $literal = std::move(literal);
            }
        ;

integer : INTEGER { $integer = std::make_pair($INTEGER, hadronParser->token($INTEGER).value.value.intValue); }
        | MINUS INTEGER %prec MINUS
            { $integer = std::make_pair($INTEGER, -hadronParser->token($INTEGER).value.value.intValue); }
        ;

float   : FLOAT { $float = std::make_pair($FLOAT, hadronParser->token($FLOAT).value.value.floatValue); }
        | MINUS FLOAT %prec MINUS
            { $float = std::make_pair($FLOAT, -hadronParser->token($FLOAT).value.value.floatValue); }
        ;

binop   : BINOP { $binop = $BINOP; }
        | PLUS { $binop = $PLUS; }
        | MINUS { $binop = $MINUS; }
        | ASTERISK { $binop = $ASTERISK; }
        | LESSTHAN { $binop = $LESSTHAN; }
        | GREATERTHAN { $binop = $GREATERTHAN; }
        | PIPE { $binop = $PIPE; }
        | READWRITEVAR { $binop = $READWRITEVAR; }
        | LEFTARROW { $binop = $LEFTARROW; }
        | KEYWORD { $binop = $KEYWORD; }
%%

yy::parser::symbol_type yylex(hadron::Parser* hadronParser) {
    auto index = hadronParser->tokenIndex();
    hadron::Token token = hadronParser->token(index);
    hadronParser->next();

    switch (token.name) {
    case hadron::Token::Name::kEmpty:
        return yy::parser::make_END(token.range);

    case hadron::Token::Name::kLiteral:
        // We special-case integers and floats to support unary negation.
        if (token.value.type == hadron::Type::kInteger) {
            return yy::parser::make_INTEGER(index, token.range);
        } else if (token.value.type == hadron::Type::kFloat) {
            return  yy::parser::make_FLOAT(index, token.range);
        }
        return yy::parser::make_LITERAL(index, token.range);

    case hadron::Token::Name::kPrimitive:
        return yy::parser::make_PRIMITIVE(index, token.range);

    case hadron::Token::Name::kPlus:
        return yy::parser::make_PLUS(index, token.range);

    case hadron::Token::Name::kMinus:
        return yy::parser::make_MINUS(index, token.range);

    case hadron::Token::Name::kAsterisk:
        return yy::parser::make_ASTERISK(index, token.range);

    case hadron::Token::Name::kAssign:
        return yy::parser::make_ASSIGN(index, token.range);

    case hadron::Token::Name::kLessThan:
        return yy::parser::make_LESSTHAN(index, token.range);

    case hadron::Token::Name::kGreaterThan:
        return yy::parser::make_GREATERTHAN(index, token.range);

    case hadron::Token::Name::kPipe:
        return yy::parser::make_PIPE(index, token.range);

    case hadron::Token::Name::kReadWriteVar:
        return yy::parser::make_READWRITEVAR(index, token.range);

    case hadron::Token::Name::kLeftArrow:
        return yy::parser::make_LEFTARROW(index, token.range);

    case hadron::Token::Name::kBinop:
        return yy::parser::make_BINOP(index, token.range);

    case hadron::Token::Name::kKeyword:
        return yy::parser::make_KEYWORD(index, token.range);

    case hadron::Token::Name::kOpenParen:
        return yy::parser::make_OPENPAREN(index, token.range);

    case hadron::Token::Name::kCloseParen:
        return yy::parser::make_CLOSEPAREN(index, token.range);

    case hadron::Token::Name::kOpenCurly:
        return yy::parser::make_OPENCURLY(index, token.range);

    case hadron::Token::Name::kCloseCurly:
        return yy::parser::make_CLOSECURLY(index, token.range);

    case hadron::Token::Name::kOpenSquare:
        return yy::parser::make_OPENSQUARE(index, token.range);

    case hadron::Token::Name::kCloseSquare:
        return yy::parser::make_CLOSESQUARE(index, token.range);

    case hadron::Token::Name::kComma:
        return yy::parser::make_COMMA(index, token.range);

    case hadron::Token::Name::kSemicolon:
        return yy::parser::make_SEMICOLON(index, token.range);

    case hadron::Token::Name::kColon:
        return yy::parser::make_COLON(index, token.range);

    case hadron::Token::Name::kCaret:
        return yy::parser::make_CARET(index, token.range);

    case hadron::Token::Name::kTilde:
        return yy::parser::make_TILDE(index, token.range);

    case hadron::Token::Name::kHash:
        return yy::parser::make_HASH(index, token.range);

    case hadron::Token::Name::kGrave:
        return yy::parser::make_GRAVE(index, token.range);

    case hadron::Token::Name::kVar:
        return yy::parser::make_VAR(index, token.range);

    case hadron::Token::Name::kArg:
        return yy::parser::make_ARG(index, token.range);

    case hadron::Token::Name::kConst:
        return yy::parser::make_CONST(index, token.range);

    case hadron::Token::Name::kClassVar:
        return yy::parser::make_CLASSVAR(index, token.range);

    case hadron::Token::Name::kIdentifier:
        return yy::parser::make_IDENTIFIER(index, token.range);

    case hadron::Token::Name::kClassName:
        return yy::parser::make_CLASSNAME(index, token.range);

    case hadron::Token::Name::kDot:
        return yy::parser::make_DOT(index, token.range);

    case hadron::Token::Name::kDotDot:
        return yy::parser::make_DOTDOT(index, token.range);

    case hadron::Token::Name::kEllipses:
        return yy::parser::make_ELLIPSES(index, token.range);

    case hadron::Token::Name::kCurryArgument:
        return yy::parser::make_CURRYARGUMENT(index, token.range);
    }

    return yy::parser::make_END(token.range);
}

void yy::parser::error(const std::string_view& location, const std::string& errorString) {
    spdlog::error(errorString);
    if (!location.empty()) {
        spdlog::error(location);
    }
    hadronParser->errorReporter()->addError(errorString);
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
//    parser.set_debug_level(1);
    auto error = parser.parse();
    if (error != 0) {
        return false;
    }

    if (!m_root) {
        m_root = std::make_unique<hadron::parse::Node>(hadron::parse::NodeType::kEmpty, 0);
    }

    return m_errorReporter->errorCount() == 0;
}

void Parser::setRoot(std::unique_ptr<parse::Node> root) {
    if (m_root) {
        m_root->append(std::move(root));
    } else {
        m_root = std::move(root);
    }
}

hadron::Token Parser::token(size_t index) const {
    if (index < m_lexer->tokens().size()) {
        return m_lexer->tokens()[index];
    } else {
        return Token();
    }
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