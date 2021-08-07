%require "3.2"
%language "c++"
%define api.value.type variant
%define api.value.automove
%define api.token.constructor
%param { hadron::Parser* hadronParser }
%skeleton "lalr1.cc"

%token LITERAL PRIMITIVE PLUS MINUS ASTERISK LESSTHAN GREATERTHAN PIPE READWRITEVAR LEFTARROW BINOP KEYWORD OPENPAREN
%token CLOSEPAREN OPENCURLY CLOSECURLY OPENSQUARE CLOSESQUARE COMMA SEMICOLON COLON CARET TILDE HASH GRAVE VAR ARG
%token CONST CLASSVAR IDENTIFIER CLASSNAME DOT DOTDOT ELLIPSES CURRYARGUMENT

%type<std::unique_ptr<hadron::parse::Node>> root funcbody expr exprn exprseq
%type<std::unique_ptr<hadron::parse::BlockNode>> cmdlinecode
%type<std::unique_ptr<hadron::parse::ClassNode>> classes classdef
%type<std::unique_ptr<hadron::parse::ReturnNode>> funretval

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
        | cmdlinecode
            { hadronParser->setRoot($cmdlinecode); }
        ;

classes[target] : { hadronParser->setRoot(nullptr); }
//                | classes[build] classdef
//                    {
//                        $build->append($classdef);
//                        $target = std::move($build);
//                    }
                ;

cmdlinecode : funcbody
                {
                    auto block = std::make_unique<hadron::parse::BlockNode>(hadronParser->tokenIndex());
                    block->body = $funcbody;
                    $cmdlinecode = std::move(block);
                }
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

optsemi :
        | ';'
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
    case hadron::Token::Name::kLiteral:
        return yy::parser::make_LITERAL();
    default:
        break;
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