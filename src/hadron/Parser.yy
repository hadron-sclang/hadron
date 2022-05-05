%require "3.2"
%language "c++"
%define api.value.type variant
%define api.token.constructor
%define parse.trace
%define parse.error verbose
%locations
%define api.location.type { hadron::Token::Location }
%param { hadron::Parser* hadronParser }
%skeleton "lalr1.cc"

%token END 0 "end of file"
%token INTERPRET
// Most tokens are just the index in the lexer's tokens() vector.
%token <size_t> LITERAL FLOAT INTEGER PRIMITIVE PLUS MINUS ASTERISK ASSIGN LESSTHAN GREATERTHAN PIPE READWRITEVAR
%token <size_t> LEFTARROW OPENPAREN CLOSEPAREN OPENCURLY CLOSECURLY OPENSQUARE CLOSESQUARE COMMA
%token <size_t> SEMICOLON COLON CARET TILDE HASH GRAVE VAR ARG CONST CLASSVAR DOT DOTDOT ELLIPSES
%token <size_t> CURRYARGUMENT IF WHILE STRING SYMBOL BINOP KEYWORD IDENTIFIER CLASSNAME BEGINCLOSEDFUNC

%type <std::unique_ptr<hadron::parse::ArgListNode>> argdecls
%type <std::unique_ptr<hadron::parse::BlockNode>> cmdlinecode block blocklist1 blocklist optblock
%type <std::unique_ptr<hadron::parse::ClassExtNode>> classextension
%type <std::unique_ptr<hadron::parse::ClassNode>> classdef
%type <std::unique_ptr<hadron::parse::ExprSeqNode>> dictslotlist1 dictslotdef arrayelems1
%type <std::unique_ptr<hadron::parse::ExprSeqNode>> exprseq methbody funcbody arglist1 arglistv1 dictslotlist arrayelems
%type <std::unique_ptr<hadron::parse::IfNode>> if
%type <std::unique_ptr<hadron::parse::WhileNode>> while
%type <std::unique_ptr<hadron::parse::KeyValueNode>> keyarg keyarglist1 optkeyarglist litdictslotdef litdictslotlist1
%type <std::unique_ptr<hadron::parse::KeyValueNode>> litdictslotlist
%type <std::unique_ptr<hadron::parse::LiteralDictNode>> dictlit2
%type <std::unique_ptr<hadron::parse::LiteralListNode>> listlit listlit2
%type <std::unique_ptr<hadron::parse::MethodNode>> methods methoddef
%type <std::unique_ptr<hadron::parse::MultiAssignVarsNode>> mavars
%type <std::unique_ptr<hadron::parse::NameNode>> mavarlist
%type <std::unique_ptr<hadron::parse::Node>> root expr exprn expr1 /* adverb */ valrangex1 msgsend literallistc
%type <std::unique_ptr<hadron::parse::Node>> literallist1 literal listliteral coreliteral classorclassext
%type <std::unique_ptr<hadron::parse::Node>> classorclassexts
%type <std::unique_ptr<hadron::parse::ReturnNode>> funretval retval
%type <std::unique_ptr<hadron::parse::SeriesIterNode>> valrange3
%type <std::unique_ptr<hadron::parse::SeriesNode>> valrange2
%type <std::unique_ptr<hadron::parse::VarDefNode>> rwslotdeflist rwslotdef slotdef vardef constdef constdeflist
%type <std::unique_ptr<hadron::parse::VarDefNode>> vardeflist slotdeflist vardeflist0 slotdeflist0
%type <std::unique_ptr<hadron::parse::VarListNode>> classvardecl classvardecls funcvardecls funcvardecl funcvardecls1
%type <std::unique_ptr<hadron::parse::StringNode>> string stringlist

%type <std::optional<size_t>> superclass optname
%type <std::optional<size_t>> primitive
%type <std::pair<bool, bool>> rwspec
%type <bool> rspec
%type <std::pair<size_t, int32_t>> integer
%type <std::pair<size_t, double>> float
%type <size_t> binop binop2

%precedence ASSIGN
%left BINOP KEYWORD PLUS MINUS ASTERISK LESSTHAN GREATERTHAN PIPE READWRITEVAR LEFTARROW
%precedence DOT
%start root

%{
#include "hadron/Parser.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Hash.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Token.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"
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
          current = YYRHSLOC(rhs, n);                                                                                  \
      } else {                                                                                                         \
          current = hadron::Token::Location{0,0};                                                                      \
      }                                                                                                                \
    }

std::ostream& operator<<(std::ostream& o, const hadron::Token::Location& loc) {
    o << loc.lineNumber + 1 << ", " << loc.characterNumber + 1;
    return o;
}

// If the expression sequence is containing a block literal this will return it, otherwise creates a new Block with the
// provided ExprSeq as the inner sequence. Used in the 'if' grammar to accept raw expressions in the true and false
// clauses, in keeping with their roots as message arguments.
std::unique_ptr<hadron::parse::BlockNode> wrapInnerBlock(std::unique_ptr<hadron::parse::ExprSeqNode>&& exprSeq) {
    if (exprSeq->expr && exprSeq->expr->nodeType == hadron::parse::kBlock) {
        auto block = reinterpret_cast<hadron::parse::BlockNode*>(exprSeq->expr.get());
        return block->moveTo();
    }

    auto block = std::make_unique<hadron::parse::BlockNode>(exprSeq->tokenIndex);
    block->body = std::move(exprSeq);
    return block;
}

yy::parser::symbol_type yylex(hadron::Parser* hadronParser);
}

%%
root    : classorclassexts { hadronParser->addRoot(std::move($classorclassexts)); }
        | INTERPRET cmdlinecode { hadronParser->addRoot(std::move($cmdlinecode)); }
        ;

classorclassexts[target]    : %empty { $target = nullptr; }
                            | classorclassexts[build] classorclassext {
                                    $target = append(std::move($build), std::move($classorclassext));
                                }
                            ;

classorclassext : classdef { $classorclassext = std::move($classdef); }
                | classextension { $classorclassext = std::move($classextension); }
                ;

classdef    : CLASSNAME superclass OPENCURLY classvardecls methods CLOSECURLY {
                    auto classDef = std::make_unique<hadron::parse::ClassNode>($CLASSNAME);
                    classDef->superClassNameIndex = $superclass;
                    classDef->variables = std::move($classvardecls);
                    classDef->methods = std::move($methods);
                    $classdef = std::move(classDef);
                }
            | CLASSNAME OPENSQUARE optname CLOSESQUARE superclass OPENCURLY classvardecls methods CLOSECURLY {
                    auto classDef = std::make_unique<hadron::parse::ClassNode>($CLASSNAME);
                    classDef->superClassNameIndex = $superclass;
                    classDef->optionalNameIndex = $optname;
                    classDef->variables = std::move($classvardecls);
                    classDef->methods = std::move($methods);
                    $classdef = std::move(classDef);
                }
            ;

classextension  : PLUS CLASSNAME OPENCURLY methods CLOSECURLY {
                        auto classExt = std::make_unique<hadron::parse::ClassExtNode>($CLASSNAME);
                        classExt->methods = std::move($methods);
                        $classextension = std::move(classExt);
                    }
                ;

optname : %empty { $optname = std::nullopt; }
        | IDENTIFIER { $optname = $IDENTIFIER; }
        ;

superclass  : %empty { $superclass = std::nullopt; }
            | COLON CLASSNAME { $superclass = $CLASSNAME; }
            ;

classvardecls[target]   : %empty { $target = nullptr; }
                        | classvardecls[build] classvardecl {
                               $target = append(std::move($build), std::move($classvardecl));
                            }
                        ;

classvardecl    : CLASSVAR rwslotdeflist SEMICOLON {
                        auto varList = std::make_unique<hadron::parse::VarListNode>($CLASSVAR);
                        varList->definitions = std::move($rwslotdeflist);
                        $classvardecl = std::move(varList);
                    }
                | VAR rwslotdeflist SEMICOLON {
                        auto varList = std::make_unique<hadron::parse::VarListNode>($VAR);
                        varList->definitions = std::move($rwslotdeflist);
                        $classvardecl = std::move(varList);
                    }
                | CONST constdeflist SEMICOLON {
                        auto varList = std::make_unique<hadron::parse::VarListNode>($CONST);
                        varList->definitions = std::move($constdeflist);
                        $classvardecl = std::move(varList);
                    }
                ;

methods[target] : %empty { $target = nullptr; }
                | methods[build] methoddef { $target = append(std::move($build), std::move($methoddef)); }
                ;

methoddef   : IDENTIFIER OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY {
                    auto method = std::make_unique<hadron::parse::MethodNode>($IDENTIFIER, false);
                    method->primitiveIndex = $primitive;
                    method->body = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                    method->body->arguments = std::move($argdecls);
                    method->body->variables = std::move($funcvardecls);
                    method->body->body = std::move($methbody);
                    $methoddef = std::move(method);
                }
            | ASTERISK IDENTIFIER OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY {
                    auto method = std::make_unique<hadron::parse::MethodNode>($IDENTIFIER, true);
                    method->primitiveIndex = $primitive;
                    method->body = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                    method->body->arguments = std::move($argdecls);
                    method->body->variables = std::move($funcvardecls);
                    method->body->body = std::move($methbody);
                    $methoddef = std::move(method);
                }
            | binop OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY {
                    auto method = std::make_unique<hadron::parse::MethodNode>($binop, false);
                    method->primitiveIndex = $primitive;
                    method->body = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                    method->body->arguments = std::move($argdecls);
                    method->body->variables = std::move($funcvardecls);
                    method->body->body = std::move($methbody);
                    $methoddef = std::move(method);
                }
            | ASTERISK binop OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY {
                    auto method = std::make_unique<hadron::parse::MethodNode>($binop, true);
                    method->primitiveIndex = $primitive;
                    method->body = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                    method->body->arguments = std::move($argdecls);
                    method->body->variables = std::move($funcvardecls);
                    method->body->body = std::move($methbody);
                    $methoddef = std::move(method);
                }
            ;

optsemi : %empty
        | SEMICOLON
        ;

optcomma    : %empty
            | COMMA
            ;

optequal    : %empty
            | ASSIGN
            ;

optblock    : %empty { $optblock = nullptr; }
            | block { $optblock = std::move($block); }
            ;

funcbody    : funretval {
                    if ($funretval) {
                        auto exprSeq = std::make_unique<hadron::parse::ExprSeqNode>($funretval->tokenIndex,
                            std::move($funretval));
                        $funcbody = std::move(exprSeq);
                    } else {
                        $funcbody = nullptr;
                    }
                }
            | exprseq funretval {
                    if ($funretval) {
                        $exprseq->expr->append(std::move($funretval));
                    }
                    $funcbody = std::move($exprseq);
                }
            ;


cmdlinecode : OPENPAREN funcvardecls1 funcbody CLOSEPAREN {
                    auto block = std::make_unique<hadron::parse::BlockNode>($OPENPAREN);
                    block->variables = std::move($funcvardecls1);
                    block->body = std::move($funcbody);
                    $cmdlinecode = std::move(block);
                }
            | funcvardecls1 funcbody {
                    auto block = std::make_unique<hadron::parse::BlockNode>($funcvardecls1->tokenIndex);
                    block->variables = std::move($funcvardecls1);
                    block->body = std::move($funcbody);
                    $cmdlinecode = std::move(block);
                }
            | funcbody {
                    // To keep the grammar unambiguous we require variable declarations in the first cmdlinecode
                    // rule, so that it doesn't collide with the definition of a expr1 as '(' exprseq ')'. So what
                    // happens to blocks that come in in parenthesis surrounding expressions without variable
                    // declaration is they match this rule for cmdlinecode, then match the expr1 rule, resulting
                    // in nested ExprSeqNodes from block. Meaning block->body is always an ExprSeqNode created by
                    // the match with the exprseq rule inside of funcbody, but then the block->body->expr node is
                    // also an ExprSeqNode, matching against the expr1 rule. We remove the redundant ExprSeqNode
                    // here.
                    if ($funcbody) {
                        auto block = std::make_unique<hadron::parse::BlockNode>($funcbody->tokenIndex);
                        if ($funcbody->expr && $funcbody->expr->nodeType == hadron::parse::NodeType::kExprSeq) {
                            hadron::parse::ExprSeqNode* exprSeq = reinterpret_cast<hadron::parse::ExprSeqNode*>(
                                $funcbody->expr.get());
                            block->body = std::make_unique<hadron::parse::ExprSeqNode>($funcbody->tokenIndex,
                                std::move(exprSeq->expr));
                        } else {
                            block->body = std::move($funcbody);
                        }
                        $cmdlinecode = std::move(block);
                    } else {
                        $cmdlinecode = nullptr;
                    }
                }
            ;

// TODO: why not merge with funcbody?
methbody    : retval {
                    if ($retval) {
                        auto exprSeq = std::make_unique<hadron::parse::ExprSeqNode>($retval->tokenIndex,
                            std::move($retval));
                        $methbody = std::move(exprSeq);
                    } else {
                        $methbody = nullptr;
                    }
                }
            | exprseq retval {
                    if ($retval) {
                        $exprseq->expr->append(std::move($retval));
                    }
                    $methbody = std::move($exprseq);
                }
            ;

primitive   : %empty { $primitive = std::nullopt; }
            | PRIMITIVE optsemi { $primitive = $PRIMITIVE; }
            ;

retval  : %empty { $retval = nullptr; }
        | CARET expr optsemi {
                auto ret = std::make_unique<hadron::parse::ReturnNode>($CARET);
                ret->valueExpr = std::move($expr);
                $retval = std::move(ret);
            }
        ;

funretval   : %empty { $funretval = nullptr; }
            | CARET expr optsemi {
                    auto ret = std::make_unique<hadron::parse::ReturnNode>($CARET);
                    ret->valueExpr = std::move($expr);
                    $funretval = std::move(ret);
                }
            ;

blocklist1[target]  : block { $target = std::move($block); }
                    | blocklist1[build] block { $target = append(std::move($build), std::move($block)); }
                    ;

blocklist   : %empty { $blocklist = nullptr; }
            | blocklist1 { $blocklist = std::move($blocklist1); }
            ;

msgsend : IDENTIFIER blocklist1 {
                auto call = std::make_unique<hadron::parse::CallNode>($IDENTIFIER);
                call->arguments = std::move($blocklist1);
                $msgsend = std::move(call);
            }
        | OPENPAREN binop2 CLOSEPAREN blocklist1 {
                auto call = std::make_unique<hadron::parse::CallNode>($binop2);
                call->arguments = std::move($blocklist1);
                $msgsend = std::move(call);
            }
        | IDENTIFIER OPENPAREN CLOSEPAREN blocklist1 {
                auto call = std::make_unique<hadron::parse::CallNode>($IDENTIFIER);
                call->arguments = std::move($blocklist1);
                $msgsend = std::move(call);
            }
        | IDENTIFIER OPENPAREN arglist1 optkeyarglist CLOSEPAREN blocklist {
                auto call = std::make_unique<hadron::parse::CallNode>($IDENTIFIER);
                call->arguments = append<std::unique_ptr<hadron::parse::Node>>(std::move($arglist1),
                    std::move($blocklist));
                call->keywordArguments = std::move($optkeyarglist);
                $msgsend = std::move(call);
            }
        | IDENTIFIER OPENPAREN arglistv1 optkeyarglist CLOSEPAREN {
                // TODO - differentiate between this and superPerformList()
                auto performList = std::make_unique<hadron::parse::PerformListNode>($OPENPAREN);
                performList->target = std::make_unique<hadron::parse::NameNode>($IDENTIFIER);
                performList->arguments = std::move($arglistv1);
                performList->keywordArguments = std::move($optkeyarglist);
                $msgsend = std::move(performList);
            }
        | CLASSNAME OPENSQUARE arrayelems CLOSESQUARE {
                // This is shorthand for Classname.new() followed by (args.add)
                auto newList = std::make_unique<hadron::parse::LiteralListNode>($OPENSQUARE);
                newList->className = std::make_unique<hadron::parse::NameNode>($CLASSNAME);
                newList->elements = std::move($arrayelems);
                $msgsend = std::move(newList);
            }
        | CLASSNAME blocklist1 {
                // This is shorthand for Classname.new {args}
                auto newCall = std::make_unique<hadron::parse::NewNode>($CLASSNAME);
                newCall->target = std::make_unique<hadron::parse::NameNode>($CLASSNAME);
                newCall->arguments = std::move($blocklist1);
                $msgsend = std::move(newCall);
            }
        | CLASSNAME OPENPAREN CLOSEPAREN blocklist {
                // This is shorthand for Classname.new() {args}
                auto newCall = std::make_unique<hadron::parse::NewNode>($CLASSNAME);
                newCall->target = std::make_unique<hadron::parse::NameNode>($CLASSNAME);
                newCall->arguments = std::move($blocklist);
                $msgsend = std::move(newCall);
            }
        | CLASSNAME OPENPAREN keyarglist1 optcomma CLOSEPAREN blocklist {
                // This is shorthand for Classname.new(foo: bazz) {args}
                auto newCall = std::make_unique<hadron::parse::NewNode>($CLASSNAME);
                newCall->target = std::make_unique<hadron::parse::NameNode>($CLASSNAME);
                newCall->arguments = std::move($blocklist);
                newCall->keywordArguments = std::move($keyarglist1);
                $msgsend = std::move(newCall);
            }
        | CLASSNAME OPENPAREN arglist1 optkeyarglist CLOSEPAREN blocklist {
                // This is shorthand for Classname.new(arg, foo: bazz) {args}
                auto newCall = std::make_unique<hadron::parse::NewNode>($CLASSNAME);
                newCall->target = std::make_unique<hadron::parse::NameNode>($CLASSNAME);
                newCall->arguments = append<std::unique_ptr<hadron::parse::Node>>(std::move($arglist1),
                        std::move($blocklist));
                newCall->keywordArguments = std::move($optkeyarglist);
                $msgsend = std::move(newCall);
            }
        | CLASSNAME OPENPAREN arglistv1 optkeyarglist CLOSEPAREN {
                auto newCall = std::make_unique<hadron::parse::NewNode>($CLASSNAME);
                newCall->target = std::make_unique<hadron::parse::NameNode>($CLASSNAME);
                newCall->arguments = std::move($arglistv1);
                newCall->keywordArguments = std::move($optkeyarglist);
                $msgsend = std::move(newCall);
            }
        | expr DOT OPENPAREN CLOSEPAREN blocklist {
                auto value = std::make_unique<hadron::parse::ValueNode>($DOT);
                value->target = std::move($expr);
                value->arguments = std::move($blocklist);
                $msgsend = std::move(value);
            }
        | expr DOT OPENPAREN keyarglist1 optcomma CLOSEPAREN blocklist {
                auto value = std::make_unique<hadron::parse::ValueNode>($DOT);
                value->target = std::move($expr);
                value->arguments = std::move($blocklist);
                value->keywordArguments = std::move($keyarglist1);
                $msgsend = std::move(value);
            }
        | expr DOT IDENTIFIER OPENPAREN keyarglist1 optcomma CLOSEPAREN blocklist {
                auto call = std::make_unique<hadron::parse::CallNode>($IDENTIFIER);
                call->target = std::move($expr);
                call->arguments = std::move($blocklist);
                call->keywordArguments = std::move($keyarglist1);
                $msgsend = std::move(call);
            }
        | expr DOT OPENPAREN arglist1 optkeyarglist CLOSEPAREN blocklist {
                auto value = std::make_unique<hadron::parse::ValueNode>($DOT);
                value->target = std::move($expr);
                value->arguments = append<std::unique_ptr<hadron::parse::Node>>(std::move($arglist1),
                        std::move($blocklist));
                value->keywordArguments = std::move($optkeyarglist);
                $msgsend = std::move(value);
            }
        | expr DOT IDENTIFIER OPENPAREN CLOSEPAREN blocklist {
                auto call = std::make_unique<hadron::parse::CallNode>($IDENTIFIER);
                call->target = std::move($expr);
                call->arguments = std::move($blocklist);
                $msgsend = std::move(call);
            }
        | expr DOT IDENTIFIER OPENPAREN arglist1 optkeyarglist CLOSEPAREN blocklist {
                auto call = std::make_unique<hadron::parse::CallNode>($IDENTIFIER);
                call->target = std::move($expr);
                call->arguments = append<std::unique_ptr<hadron::parse::Node>>(std::move($arglist1),
                    std::move($blocklist));
                call->keywordArguments = std::move($optkeyarglist);
                $msgsend = std::move(call);
            }
        | expr DOT IDENTIFIER OPENPAREN arglistv1 optkeyarglist CLOSEPAREN {
                // TODO - differentiate between this and superPerformList()
                auto performList = std::make_unique<hadron::parse::PerformListNode>($IDENTIFIER);
                performList->target = std::move($expr);
                performList->arguments = std::move($arglistv1);
                performList->keywordArguments = std::move($optkeyarglist);
                $msgsend = std::move(performList);
            }
        | expr DOT IDENTIFIER blocklist {
                auto call = std::make_unique<hadron::parse::CallNode>($IDENTIFIER);
                call->target = std::move($expr);
                call->arguments = std::move($blocklist);
                $msgsend = std::move(call);
            }
        ;

if  : IF OPENPAREN exprseq[condition] COMMA exprseq[true] COMMA exprseq[false] optcomma CLOSEPAREN {
            auto ifNode = std::make_unique<hadron::parse::IfNode>($IF);
            ifNode->condition = std::move($condition);
            ifNode->trueBlock = wrapInnerBlock(std::move($true));
            ifNode->falseBlock = wrapInnerBlock(std::move($false));
            $if = std::move(ifNode);
        }
    | IF OPENPAREN exprseq[condition] COMMA exprseq[true] optcomma CLOSEPAREN {
            auto ifNode = std::make_unique<hadron::parse::IfNode>($IF);
            ifNode->condition = std::move($condition);
            ifNode->trueBlock = wrapInnerBlock(std::move($true));
            $if = std::move(ifNode);
        }
    | expr DOT IF OPENPAREN exprseq[true] COMMA exprseq[false] optcomma CLOSEPAREN {
            auto ifNode = std::make_unique<hadron::parse::IfNode>($IF);
            ifNode->condition = std::make_unique<hadron::parse::ExprSeqNode>($expr->tokenIndex, std::move($expr));
            ifNode->trueBlock = wrapInnerBlock(std::move($true));
            ifNode->falseBlock = wrapInnerBlock(std::move($false));
            $if = std::move(ifNode);
        }
    | expr DOT IF OPENPAREN exprseq[true] optcomma CLOSEPAREN {
            auto ifNode = std::make_unique<hadron::parse::IfNode>($IF);
            ifNode->condition = std::make_unique<hadron::parse::ExprSeqNode>($expr->tokenIndex, std::move($expr));
            ifNode->trueBlock = wrapInnerBlock(std::move($true));
            $if = std::move(ifNode);
        }
    | IF OPENPAREN exprseq[condition] CLOSEPAREN block[true] optblock {
            auto ifNode = std::make_unique<hadron::parse::IfNode>($IF);
            ifNode->condition = std::move($condition);
            ifNode->trueBlock = std::move($true);
            ifNode->falseBlock = std::move($optblock);
            $if = std::move(ifNode);
        }
    ;

while   : WHILE OPENPAREN block[condition] optcomma blocklist[blocks] CLOSEPAREN {
                auto whileNode = std::make_unique<hadron::parse::WhileNode>($WHILE);
                whileNode->blocks = append<std::unique_ptr<hadron::parse::BlockNode>>(std::move($condition),
                        std::move($blocks));
                $while = std::move(whileNode);
            }
        | WHILE blocklist1[blocks] {
                auto whileNode = std::make_unique<hadron::parse::WhileNode>($WHILE);
                whileNode->blocks = std::move($blocks);
                $while = std::move(whileNode);
            }
        ;

// TODO: figure out is this used? What does it mean? Things like: "4 + .foo 5" parse in LSC (evaluates to 9).
/*
adverb  : %empty { $adverb = nullptr; }
        | DOT IDENTIFIER {
                $adverb = std::make_unique<hadron::parse::LiteralNode($IDENTIFIER,
                    hadron::Slot(hadron::Type::kSymbol, hadron::Slot::Value()));
            }
        | DOT INTEGER {
                $adverb = std::make_unique<hadron::parse::LiteralNode($INTEGER.first,
                    hadron::Slot(hadron::Type::kInteger, hadron::Slot::Value($INTEGER.second)));
            }
        | DOT OPENPAREN exprseq CLOSEPAREN { $adverb = std::move($exprseq); }
        ;
*/

exprn[target]   : expr { $target = std::move($expr); }
                | exprn[build] SEMICOLON expr { $target = append(std::move($build), std::move($expr)); }
                ;

exprseq : exprn optsemi {
                $exprseq = std::make_unique<hadron::parse::ExprSeqNode>($exprn->tokenIndex, std::move($exprn));
            }
        ;

arrayelems  : %empty { $arrayelems = nullptr; }
            | arrayelems1 optcomma { $arrayelems = std::move($arrayelems1); }
            ;

arrayelems1[target] : exprseq { $target = std::move($exprseq); }
                    | exprseq[build] COLON exprseq[next] { $target = append(std::move($build), std::move($next)); }
                    | KEYWORD exprseq {
                            auto symbol = std::make_unique<hadron::parse::SymbolNode>($KEYWORD);
                            auto exprSeq = std::make_unique<hadron::parse::ExprSeqNode>($KEYWORD, std::move(symbol));
                            $target = append(std::move(exprSeq), std::move($exprseq));
                        }
                    | arrayelems1[build] COMMA exprseq {
                            $target = append(std::move($build), std::move($exprseq));
                        }
                    | arrayelems1[build] COMMA KEYWORD exprseq {
                            auto symbol = std::make_unique<hadron::parse::SymbolNode>($KEYWORD);
                            auto exprSeq = std::make_unique<hadron::parse::ExprSeqNode>($KEYWORD, std::move(symbol));
                            auto affixes = append(std::move(exprSeq), std::move($exprseq));
                            $target = append(std::move($build), std::move(affixes));
                        }
                    | arrayelems1[build] COMMA exprseq[append] COLON exprseq[next] {
                        auto affixes = append(std::move($append), std::move($next));
                        $target = append(std::move($build), std::move(affixes));
                        }
                    ;

expr1[target]   : literal { $target = std::move($literal); }
                | IDENTIFIER { $target = std::make_unique<hadron::parse::NameNode>($IDENTIFIER); }
                | CURRYARGUMENT { $target = std::make_unique<hadron::parse::CurryArgumentNode>($CURRYARGUMENT); }
                | msgsend { $target = std::move($msgsend); }
                | OPENPAREN exprseq CLOSEPAREN {
                        // To keep consistent with variable-less cmdlinecode blocks that get parsed as this we point
                        // to the openening parenthesis with the tokenIndex.
                        $exprseq->tokenIndex = $OPENPAREN;
                        $target = std::move($exprseq);
                    }
                | TILDE IDENTIFIER {
                        auto envirAt = std::make_unique<hadron::parse::EnvironmentAtNode>($IDENTIFIER);
                        $target = std::move(envirAt);
                    }
                | OPENSQUARE arrayelems CLOSESQUARE {
                        auto array = std::make_unique<hadron::parse::ArrayNode>($OPENSQUARE);
                        array->elements = std::move($arrayelems);
                        $target = std::move(array);
                    }
                | OPENPAREN valrange2 CLOSEPAREN { $target = std::move($valrange2); }
                | OPENPAREN COLON valrange3 CLOSEPAREN { $target = std::move($valrange3); }
                | OPENPAREN dictslotlist CLOSEPAREN {
                        auto event = std::make_unique<hadron::parse::EventNode>($OPENPAREN);
                        event->elements = std::move($dictslotlist);
                        $target = std::move(event);
                    }
                | expr1[build] OPENSQUARE arglist1 CLOSESQUARE {
                        auto arrayNode = std::make_unique<hadron::parse::ArrayReadNode>($OPENSQUARE);
                        arrayNode->targetArray = std::move($build);
                        arrayNode->indexArgument = std::move($arglist1);
                        $target = std::move(arrayNode);
                    }
                | valrangex1 { $target = std::move($valrangex1); }
                | if { $target = std::move($if); }
                | while { $target = std::move($while); }
                ;

valrangex1  : expr1 OPENSQUARE arglist1 DOTDOT CLOSESQUARE {
                    auto copySeries = std::make_unique<hadron::parse::CopySeriesNode>($OPENSQUARE);
                    copySeries->target = std::move($expr1);
                    copySeries->first = std::move($arglist1);
                    if (copySeries->first->next) {
                        copySeries->second = std::move(copySeries->first->next);
                        copySeries->first->next = nullptr;
                    }
                    $valrangex1 = std::move(copySeries);
                }
            | expr1 OPENSQUARE DOTDOT exprseq[last] CLOSESQUARE {
                    auto copySeries = std::make_unique<hadron::parse::CopySeriesNode>($OPENSQUARE);
                    copySeries->target = std::move($expr1);
                    copySeries->last = std::move($last);
                    $valrangex1 = std::move(copySeries);
                }
            | expr1 OPENSQUARE arglist1 DOTDOT exprseq[last] CLOSESQUARE {
                    auto copySeries = std::make_unique<hadron::parse::CopySeriesNode>($OPENSQUARE);
                    copySeries->target = std::move($expr1);
                    copySeries->first = std::move($arglist1);
                    if (copySeries->first->next) {
                        copySeries->second = std::move(copySeries->first->next);
                        copySeries->first->next = nullptr;
                    }
                    copySeries->last = std::move($last);
                    $valrangex1 = std::move(copySeries);
                }
            ;

// (start, step..size) --> SimpleNumber.series(start, step, last) -> start.series(step, last)
valrange2   : exprseq[start] DOTDOT {
                    // There are only certain contexts that this is a valid construction, a 'do' operation and a list
                    // comprehension. (Where the last value is implied)
                    auto series = std::make_unique<hadron::parse::SeriesNode>($DOTDOT);
                    series->start = std::move($start);
                    $valrange2 = std::move(series);
                }
            | DOTDOT exprseq[last] {
                    auto series = std::make_unique<hadron::parse::SeriesNode>($DOTDOT);
                    series->last = std::move($last);
                    $valrange2 = std::move(series);
                }
            | exprseq[start] DOTDOT exprseq[last] {
                    auto series = std::make_unique<hadron::parse::SeriesNode>($DOTDOT);
                    series->start = std::move($start);
                    series->last = std::move($last);
                    $valrange2 = std::move(series);
                }
            | exprseq[start] COMMA exprseq[step] DOTDOT {
                    // Another case that needs an implied last value.
                    auto series = std::make_unique<hadron::parse::SeriesNode>($DOTDOT);
                    series->start = std::move($start);
                    series->step = std::move($step);
                    $valrange2 = std::move(series);
                }
            | exprseq[start] COMMA exprseq[step] DOTDOT exprseq[last] {
                    auto series = std::make_unique<hadron::parse::SeriesNode>($DOTDOT);
                    series->start = std::move($start);
                    series->step = std::move($step);
                    series->last = std::move($last);
                    $valrange2 = std::move(series);
                }
            ;

valrange3   : exprseq[start] DOTDOT {
                    auto seriesIter = std::make_unique<hadron::parse::SeriesIterNode>($DOTDOT);
                    seriesIter->start = std::move($start);
                    $valrange3 = std::move(seriesIter);
                }
            | DOTDOT exprseq[last] {
                    auto seriesIter = std::make_unique<hadron::parse::SeriesIterNode>($DOTDOT);
                    seriesIter->last = std::move($last);
                    $valrange3 = std::move(seriesIter);
                }
            | exprseq[start] DOTDOT exprseq[last] {
                    auto seriesIter = std::make_unique<hadron::parse::SeriesIterNode>($DOTDOT);
                    seriesIter->start = std::move($start);
                    seriesIter->last = std::move($last);
                    $valrange3 = std::move(seriesIter);
                }
            | exprseq[start] COMMA exprseq[step] DOTDOT {
                    auto seriesIter = std::make_unique<hadron::parse::SeriesIterNode>($DOTDOT);
                    seriesIter->start = std::move($start);
                    seriesIter->step = std::move($step);
                    $valrange3 = std::move(seriesIter);
                }
            | exprseq[start] COMMA exprseq[step] DOTDOT exprseq[last] {
                    auto seriesIter = std::make_unique<hadron::parse::SeriesIterNode>($DOTDOT);
                    seriesIter->start = std::move($start);
                    seriesIter->step = std::move($step);
                    seriesIter->last = std::move($last);
                    $valrange3 = std::move(seriesIter);
                }
            ;

expr[target]    : expr1 { $target = std::move($expr1); }
                | CLASSNAME { $target = std::make_unique<hadron::parse::NameNode>($CLASSNAME); }
                | expr[build] binop2 /* adverb */ expr[rightHand] %prec BINOP {
                        auto binop = std::make_unique<hadron::parse::BinopCallNode>($binop2);
                        binop->leftHand = std::move($build);
                        binop->rightHand = std::move($rightHand);
                        /* binop->adverb = std::move($adverb); */
                        $target = std::move(binop);
                    }
                | IDENTIFIER ASSIGN expr[build] {
                        auto assign = std::make_unique<hadron::parse::AssignNode>($ASSIGN);
                        assign->name = std::make_unique<hadron::parse::NameNode>($IDENTIFIER);
                        assign->value = std::move($build);
                        $target = std::move(assign);
                    }
                | TILDE IDENTIFIER ASSIGN expr[build] {
                        auto envirPut = std::make_unique<hadron::parse::EnvironmentPutNode>($IDENTIFIER);
                        envirPut->value = std::move($build);
                        $target = std::move(envirPut);
                    }
                | expr[build] DOT IDENTIFIER ASSIGN expr[value] {
                        auto setter = std::make_unique<hadron::parse::SetterNode>($IDENTIFIER);
                        setter->target = std::move($build);
                        setter->value = std::move($value);
                        $target = std::move(setter);
                    }
                | IDENTIFIER OPENPAREN arglist1 optkeyarglist CLOSEPAREN ASSIGN expr[value] {
                        if ($optkeyarglist != nullptr) {
                            // TODO: LSC error condition "Setter method called with keyword arguments"
                        }
                        auto setter = std::make_unique<hadron::parse::SetterNode>($IDENTIFIER);
                        setter->target = std::move($arglist1);
                        setter->value = std::move($value);
                    }
                | HASH mavars ASSIGN expr[value] {
                        auto multiAssign = std::make_unique<hadron::parse::MultiAssignNode>($ASSIGN);
                        multiAssign->targets = std::move($mavars);
                        multiAssign->value = std::move($value);
                        $target = std::move(multiAssign);
                    }
                | expr1 OPENSQUARE arglist1 CLOSESQUARE ASSIGN expr[value] {
                        auto write = std::make_unique<hadron::parse::ArrayWriteNode>($ASSIGN);
                        write->targetArray = std::move($expr1);
                        write->indexArgument = std::move($arglist1);
                        write->value = std::move($value);
                        $target = std::move(write);
                    }
                ;

block   : OPENCURLY argdecls funcvardecls methbody CLOSECURLY {
                auto block = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                block->arguments = std::move($argdecls);
                block->variables = std::move($funcvardecls);
                block->body = std::move($methbody);
                $block = std::move(block);
            }
        | BEGINCLOSEDFUNC argdecls funcvardecls funcbody CLOSECURLY {
                auto block = std::make_unique<hadron::parse::BlockNode>($BEGINCLOSEDFUNC);
                block->arguments = std::move($argdecls);
                block->variables = std::move($funcvardecls);
                block->body = std::move($funcbody);
                $block = std::move(block);
            }
        ;

funcvardecls[target]    : %empty { $target = nullptr; }
                        | funcvardecls[build] funcvardecl {
                                $target = append(std::move($build), std::move($funcvardecl));
                            }
                        ;

funcvardecls1[target]   : funcvardecl { $target = std::move($funcvardecl); }
                        | funcvardecls1[build] funcvardecl {
                                $target = append(std::move($build), std::move($funcvardecl));
                            }
                        ;

funcvardecl : VAR vardeflist SEMICOLON {
                    auto varList = std::make_unique<hadron::parse::VarListNode>($VAR);
                    varList->definitions = std::move($vardeflist);
                    $funcvardecl = std::move(varList);
                }
            ;

argdecls    : %empty { $argdecls = nullptr; }
            | ARG vardeflist SEMICOLON {
                    auto argList = std::make_unique<hadron::parse::ArgListNode>($ARG);
                    argList->varList = std::make_unique<hadron::parse::VarListNode>($ARG);
                    argList->varList->definitions = std::move($vardeflist);
                    $argdecls = std::move(argList);
                }
            | ARG vardeflist0 ELLIPSES IDENTIFIER SEMICOLON {
                    auto argList = std::make_unique<hadron::parse::ArgListNode>($ARG);
                    argList->varList = std::make_unique<hadron::parse::VarListNode>($ARG);
                    argList->varList->definitions = std::move($vardeflist0);
                    argList->varArgsNameIndex = $IDENTIFIER;
                    $argdecls = std::move(argList);
                }
            | PIPE[open] slotdeflist PIPE {
                    auto argList = std::make_unique<hadron::parse::ArgListNode>($open);
                    argList->varList = std::make_unique<hadron::parse::VarListNode>($open);
                    argList->varList->definitions = std::move($slotdeflist);
                    $argdecls = std::move(argList);
                }
            | PIPE[open] slotdeflist0 ELLIPSES IDENTIFIER PIPE {
                    auto argList = std::make_unique<hadron::parse::ArgListNode>($open);
                    argList->varList = std::make_unique<hadron::parse::VarListNode>($open);
                    argList->varList->definitions = std::move($slotdeflist0);
                    argList->varArgsNameIndex = $IDENTIFIER;
                    $argdecls = std::move(argList);
                }
            ;

constdeflist[target]    : constdef { $target = std::move($constdef); }
                        | constdeflist[build] optcomma constdef {
                                $target = append(std::move($build), std::move($constdef));
                            }
                        ;

constdef    : rspec IDENTIFIER ASSIGN literal {
                    auto constDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                    constDef->hasReadAccessor = $rspec;
                    constDef->initialValue = std::move($literal);
                    $constdef = std::move(constDef);
                }
            ;

slotdeflist0    : %empty { $slotdeflist0 = nullptr; }
                | slotdeflist { $slotdeflist0 = std::move($slotdeflist); }
                ;

slotdeflist[target] : slotdef { $target = std::move($slotdef); }
                    | slotdeflist[build] optcomma slotdef { $target = append(std::move($build), std::move($slotdef)); }
                    ;

slotdef : IDENTIFIER { $slotdef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER); }
        | IDENTIFIER optequal literal {
                auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                varDef->initialValue = std::move($literal);
                $slotdef = std::move(varDef);
            }
        | IDENTIFIER optequal OPENPAREN exprseq CLOSEPAREN {
                auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                varDef->initialValue = std::move($exprseq);
                $slotdef = std::move(varDef);
            }
        ;

vardeflist0 : %empty { $vardeflist0 = nullptr; }
            | vardeflist { $vardeflist0 = std::move($vardeflist); }
            ;

vardeflist[target]  : vardef { $target = std::move($vardef); }
                    | vardeflist[build] COMMA vardef { $target = append(std::move($build), std::move($vardef)); }
                    ;

vardef  : IDENTIFIER { $vardef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER); }
        | IDENTIFIER ASSIGN expr {
                auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                varDef->initialValue = std::move($expr);
                $vardef = std::move(varDef);
            }
        | IDENTIFIER OPENPAREN exprseq CLOSEPAREN {
                auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                varDef->initialValue = std::move($exprseq);
                $vardef = std::move(varDef);
            }
        ;

dictslotdef  : exprseq[build] COLON exprseq[next] { $dictslotdef = append(std::move($build), std::move($next)); }
             | KEYWORD exprseq {
                    auto symbol = std::make_unique<hadron::parse::SymbolNode>($KEYWORD);
                    auto exprSeq = std::make_unique<hadron::parse::ExprSeqNode>($KEYWORD, std::move(symbol));
                    $dictslotdef = append(std::move(exprSeq), std::move($exprseq));
                }
             ;

dictslotlist1[target]   : dictslotdef { $target = std::move($dictslotdef); }
                        | dictslotlist1[build] COMMA dictslotdef {
                                $target = append(std::move($build), std::move($dictslotdef));
                            }
                        ;

dictslotlist    : %empty { $dictslotlist = nullptr; }
                | dictslotlist1 optcomma { $dictslotlist = std::move($dictslotlist1); }
                ;

dictlit2: OPENPAREN litdictslotlist CLOSEPAREN {
                auto litDict = std::make_unique<hadron::parse::LiteralDictNode>($OPENPAREN);
                litDict->elements = std::move($litdictslotlist);
                $dictlit2 = std::move(litDict);
            }
        ;

litdictslotdef  : listliteral[key] ':' listliteral[value] {
                        auto keyValue = std::make_unique<hadron::parse::KeyValueNode>($key->tokenIndex);
                        keyValue->key = std::move($key);
                        keyValue->value = std::move($value);
                        $litdictslotdef = std::move(keyValue);
                    }
                | KEYWORD listliteral {
                        auto keyValue = std::make_unique<hadron::parse::KeyValueNode>($KEYWORD);
                        keyValue->key = std::make_unique<hadron::parse::SymbolNode>($KEYWORD);
                        keyValue->value = std::move($listliteral);
                        $litdictslotdef = std::move(keyValue);
                    }
                ;

litdictslotlist1[target]    : litdictslotdef { $target = std::move($litdictslotdef); }
                            | litdictslotlist1[build] ',' litdictslotdef {
                                    $target = append(std::move($build), std::move($litdictslotdef));
                                }
                            ;

litdictslotlist : %empty { $litdictslotlist = nullptr; }
                | litdictslotlist1 optcomma { $litdictslotlist = std::move($litdictslotlist1); }
                ;


listlit : HASH listlit2 { $listlit = std::move($listlit2); }
        ;

// Same as listlit but without the hashes, for inner literal lists
listlit2 : OPENSQUARE literallistc CLOSESQUARE {
                auto litList = std::make_unique<hadron::parse::LiteralListNode>($OPENSQUARE);
                litList->elements = std::move($literallistc);
                $listlit2 = std::move(litList);
            }
        | CLASSNAME OPENSQUARE literallistc CLOSESQUARE {
                auto litList = std::make_unique<hadron::parse::LiteralListNode>($OPENSQUARE);
                litList->className = std::make_unique<hadron::parse::NameNode>($CLASSNAME);
                litList->elements = std::move($literallistc);
                $listlit2 = std::move(litList);
            }
        ;

literallistc    : %empty { $literallistc = nullptr; }
                | literallist1 optcomma { $literallistc = std::move($literallist1); }
                ;

literallist1[target]    : listliteral { $target = std::move($listliteral); }
                        | literallist1[build] COMMA listliteral {
                                $target = append(std::move($build), std::move($listliteral));
                            }
                        ;

rwslotdeflist[target]   : rwslotdef { $target = std::move($rwslotdef); }
                        | rwslotdeflist[build] COMMA rwslotdef {
                                $target = append(std::move($build), std::move($rwslotdef));
                            }
                        ;

rwslotdef   : rwspec IDENTIFIER {
                    auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                    varDef->hasReadAccessor = $rwspec.first;
                    varDef->hasWriteAccessor = $rwspec.second;
                    $rwslotdef = std::move(varDef);
                }
            | rwspec IDENTIFIER ASSIGN literal {
                    auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER);
                    varDef->hasReadAccessor = $rwspec.first;
                    varDef->hasWriteAccessor = $rwspec.second;
                    varDef->initialValue = std::move($literal);
                    $rwslotdef = std::move(varDef);
                }
            ;

rwspec  : %empty { $rwspec = std::make_pair(false, false); }
        | LESSTHAN { $rwspec = std::make_pair(true, false); }
        | GREATERTHAN { $rwspec = std::make_pair(false, true); }
        | READWRITEVAR { $rwspec = std::make_pair(true, true); }
        ;

rspec   : %empty { $rspec = false; }
        | LESSTHAN { $rspec = true; }
        ;

arglist1[target]    : exprseq { $target = std::move($exprseq); }
                    | arglist1[build] COMMA exprseq { $target = append(std::move($build), std::move($exprseq)); }
                    ;

arglistv1   : ASTERISK exprseq { $arglistv1 = std::move($exprseq); }
            | arglist1 COMMA ASTERISK exprseq { $arglistv1 = append(std::move($arglist1), std::move($exprseq)); }
            ;

keyarglist1[target] : keyarg { $target = std::move($keyarg); }
                    | keyarglist1[build] COMMA keyarg { $target = append(std::move($build), std::move($keyarg)); }
                    ;

keyarg  : KEYWORD exprseq {
                auto keyArg = std::make_unique<hadron::parse::KeyValueNode>($KEYWORD);
                keyArg->key = std::make_unique<hadron::parse::SymbolNode>($KEYWORD);
                keyArg->value = std::move($exprseq);
                $keyarg = std::move(keyArg);
            }
        ;

optkeyarglist   : optcomma { $optkeyarglist = nullptr; }
                | COMMA keyarglist1 optcomma { $optkeyarglist = std::move($keyarglist1); }
                ;

mavars  : mavarlist {
                auto multiVars = std::make_unique<hadron::parse::MultiAssignVarsNode>($mavarlist->tokenIndex);
                multiVars->names = std::move($mavarlist);
                $mavars = std::move(multiVars);
            }
        | mavarlist ELLIPSES IDENTIFIER {
                auto multiVars = std::make_unique<hadron::parse::MultiAssignVarsNode>($mavarlist->tokenIndex);
                multiVars->names = std::move($mavarlist);
                multiVars->rest = std::make_unique<hadron::parse::NameNode>($IDENTIFIER);
                $mavars = std::move(multiVars);
            }
        ;

mavarlist[target]   : IDENTIFIER {
                            auto name = std::make_unique<hadron::parse::NameNode>($IDENTIFIER);
                            $target = std::move(name);
                        }
                    | mavarlist[build] COMMA IDENTIFIER {
                            auto name = std::make_unique<hadron::parse::NameNode>($IDENTIFIER);
                            $target = append(std::move($build), std::move(name));
                        }
                    ;

listliteral : coreliteral { $listliteral = std::move($coreliteral); }
        | listlit2 { $listliteral = std::move($listlit2); }
        | dictlit2 { $listliteral = std::move($dictlit2); }
        // NOTE: within lists symbols can be specified without \ or '' notation.
        | IDENTIFIER { $listliteral = std::make_unique<hadron::parse::SymbolNode>($IDENTIFIER); }
        ;

literal : coreliteral { $literal = std::move($coreliteral); }
        | listlit { $literal = std::move($listlit); }
        ;

coreliteral : LITERAL {
                    $coreliteral = std::make_unique<hadron::parse::SlotNode>($LITERAL,
                            hadronParser->token($LITERAL).value);
                }
            | integer {
                    $coreliteral = std::make_unique<hadron::parse::SlotNode>($integer.first,
                            hadron::Slot::makeInt32($integer.second));
                }
            | float {
                    $coreliteral = std::make_unique<hadron::parse::SlotNode>($float.first,
                            hadron::Slot::makeFloat($float.second));
                }
            | block { $coreliteral = std::move($block); }
            | stringlist { $coreliteral = std::move($stringlist); }
            | SYMBOL { $coreliteral = std::make_unique<hadron::parse::SymbolNode>($SYMBOL); }
            ;

integer : INTEGER { $integer = std::make_pair($INTEGER, hadronParser->token($INTEGER).value.getInt32()); }
        | MINUS INTEGER %prec MINUS {
                $integer = std::make_pair($INTEGER, -hadronParser->token($INTEGER).value.getInt32());
            }
        ;

float   : FLOAT { $float = std::make_pair($FLOAT, hadronParser->token($FLOAT).value.getFloat()); }
        | MINUS FLOAT %prec MINUS {
                $float = std::make_pair($FLOAT, -hadronParser->token($FLOAT).value.getFloat());
            }
        ;

binop   : BINOP { $binop = $BINOP; }
        | PLUS { $binop = $PLUS; }
        | MINUS { $binop = $MINUS; }
        | ASTERISK { $binop = $ASTERISK; }
        | LESSTHAN { $binop = $LESSTHAN; }
        | GREATERTHAN { $binop = $GREATERTHAN; }
        | PIPE { $binop = $PIPE; }
        | READWRITEVAR { $READWRITEVAR; }
        | LEFTARROW { $LEFTARROW; }
        ;

binop2  : binop { $binop2 = $binop; }
        | KEYWORD { $binop2 = $KEYWORD; }
        ;

string  : STRING { $string = std::make_unique<hadron::parse::StringNode>($STRING); }
        ;

stringlist[target]  : string { $target = std::move($string); }
                    | stringlist[build] string {
                            $target = append(std::move($build), std::move($string));
                        }
                    ;
%%

yy::parser::symbol_type yylex(hadron::Parser* hadronParser) {
    if (hadronParser->sendInterpret()) {
        hadronParser->setInterpret(false);
        return yy::parser::make_INTERPRET(hadron::Token::Location{0, 0});
    }

    auto index = hadronParser->tokenIndex();
    hadron::Token token = hadronParser->token(index);
    hadronParser->next();

    switch (token.name) {
    case hadron::Token::Name::kEmpty:
        return yy::parser::make_END(token.location);

    case hadron::Token::Name::kInterpret:
        return yy::parser::make_INTERPRET(token.location);

    case hadron::Token::Name::kLiteral:
        // We special-case integers and floats to support unary negation.
        if (token.value.isInt32()) {
            return yy::parser::make_INTEGER(index, token.location);
        } else if (token.value.isFloat()) {
            return  yy::parser::make_FLOAT(index, token.location);
        }
        return yy::parser::make_LITERAL(index, token.location);

    case hadron::Token::Name::kString:
        return yy::parser::make_STRING(index, token.location);

    case hadron::Token::Name::kSymbol:
        return yy::parser::make_SYMBOL(index, token.location);

    case hadron::Token::Name::kPrimitive:
        return yy::parser::make_PRIMITIVE(index, token.location);

    case hadron::Token::Name::kPlus:
        return yy::parser::make_PLUS(index, token.location);

    case hadron::Token::Name::kMinus:
        return yy::parser::make_MINUS(index, token.location);

    case hadron::Token::Name::kAsterisk:
        return yy::parser::make_ASTERISK(index, token.location);

    case hadron::Token::Name::kAssign:
        return yy::parser::make_ASSIGN(index, token.location);

    case hadron::Token::Name::kLessThan:
        return yy::parser::make_LESSTHAN(index, token.location);

    case hadron::Token::Name::kGreaterThan:
        return yy::parser::make_GREATERTHAN(index, token.location);

    case hadron::Token::Name::kPipe:
        return yy::parser::make_PIPE(index, token.location);

    case hadron::Token::Name::kReadWriteVar:
        return yy::parser::make_READWRITEVAR(index, token.location);

    case hadron::Token::Name::kLeftArrow:
        return yy::parser::make_LEFTARROW(index, token.location);

    case hadron::Token::Name::kBinop:
        return yy::parser::make_BINOP(index, token.location);

    case hadron::Token::Name::kKeyword:
        return yy::parser::make_KEYWORD(index, token.location);

    case hadron::Token::Name::kOpenParen:
        return yy::parser::make_OPENPAREN(index, token.location);

    case hadron::Token::Name::kCloseParen:
        return yy::parser::make_CLOSEPAREN(index, token.location);

    case hadron::Token::Name::kOpenCurly:
        return yy::parser::make_OPENCURLY(index, token.location);

    case hadron::Token::Name::kCloseCurly:
        return yy::parser::make_CLOSECURLY(index, token.location);

    case hadron::Token::Name::kOpenSquare:
        return yy::parser::make_OPENSQUARE(index, token.location);

    case hadron::Token::Name::kCloseSquare:
        return yy::parser::make_CLOSESQUARE(index, token.location);

    case hadron::Token::Name::kComma:
        return yy::parser::make_COMMA(index, token.location);

    case hadron::Token::Name::kSemicolon:
        return yy::parser::make_SEMICOLON(index, token.location);

    case hadron::Token::Name::kColon:
        return yy::parser::make_COLON(index, token.location);

    case hadron::Token::Name::kCaret:
        return yy::parser::make_CARET(index, token.location);

    case hadron::Token::Name::kTilde:
        return yy::parser::make_TILDE(index, token.location);

    case hadron::Token::Name::kHash:
        return yy::parser::make_HASH(index, token.location);

    case hadron::Token::Name::kGrave:
        return yy::parser::make_GRAVE(index, token.location);

    case hadron::Token::Name::kVar:
        return yy::parser::make_VAR(index, token.location);

    case hadron::Token::Name::kArg:
        return yy::parser::make_ARG(index, token.location);

    case hadron::Token::Name::kConst:
        return yy::parser::make_CONST(index, token.location);

    case hadron::Token::Name::kClassVar:
        return yy::parser::make_CLASSVAR(index, token.location);

    case hadron::Token::Name::kIdentifier:
        return yy::parser::make_IDENTIFIER(index, token.location);

    case hadron::Token::Name::kClassName:
        return yy::parser::make_CLASSNAME(index, token.location);

    case hadron::Token::Name::kDot:
        return yy::parser::make_DOT(index, token.location);

    case hadron::Token::Name::kDotDot:
        return yy::parser::make_DOTDOT(index, token.location);

    case hadron::Token::Name::kEllipses:
        return yy::parser::make_ELLIPSES(index, token.location);

    case hadron::Token::Name::kCurryArgument:
        return yy::parser::make_CURRYARGUMENT(index, token.location);

    case hadron::Token::Name::kBeginClosedFunction:
        return yy::parser::make_BEGINCLOSEDFUNC(index, token.location);

    case hadron::Token::Name::kIf:
        return yy::parser::make_IF(index, token.location);

    case hadron::Token::Name::kWhile:
        return yy::parser::make_WHILE(index, token.location);
    }

    return yy::parser::make_END(token.location);
}

void yy::parser::error(const hadron::Token::Location& location, const std::string& errorString) {
    auto locError = fmt::format("line {} col {}: {}", location.lineNumber + 1, location.characterNumber + 1,
            errorString);
    hadronParser->errorReporter()->addError(locError);
}

namespace hadron {

Parser::Parser(Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter):
    m_lexer(lexer),
    m_tokenIndex(0),
    m_sendInterpret(false),
    m_threadContext(nullptr),
    m_errorReporter(errorReporter) { }

Parser::Parser(std::string_view code):
    m_ownLexer(std::make_unique<Lexer>(code)),
    m_lexer(m_ownLexer.get()),
    m_tokenIndex(0),
    m_sendInterpret(false),
    m_threadContext(nullptr),
    m_errorReporter(m_ownLexer->errorReporter()) { }

Parser::~Parser() {}

bool Parser::parse(ThreadContext* context) {
    m_sendInterpret = true;
    return innerParse(context);
}

bool Parser::parseClass() {
    m_sendInterpret = false;
    return innerParse(context);
}

void Parser::addRoot(std::unique_ptr<parse::Node> root) {
    if (root) {
        if (m_root) {
            m_root->append(std::move(root));
        } else {
            m_root = std::move(root);
        }
    }
}

hadron::Token Parser::token(size_t index) const {
    if (index < m_lexer->tokens().size()) {
        return m_lexer->tokens()[index];
    } else {
        return Token::makeEmpty();
    }
}

bool Parser::innerParse(ThreadContext* context) {
    m_threadContext = context;

    if (m_ownLexer) {
        if (!m_lexer->lex()) {
            return false;
        }
    }

    m_root = nullptr;
    m_tokenIndex = 0;
    yy::parser parser(this);
    auto error = parser.parse();
    if (error != 0) {
        return false;
    }

    // Valid parse of empty input buffer, which we represent with an empty node.
    if (!m_root) {
        m_root = library::EmptyNode::alloc(context);
    }

    return m_errorReporter->errorCount() == 0;
}

} // namespace hadron