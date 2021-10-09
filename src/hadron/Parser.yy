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
%token <size_t> CURRYARGUMENT IF
// Include Hashes in the token when meaningful.
%token <std::pair<size_t, hadron::Hash>> BINOP KEYWORD IDENTIFIER CLASSNAME

%type <std::unique_ptr<hadron::parse::ArgListNode>> argdecls
%type <std::unique_ptr<hadron::parse::BlockNode>> cmdlinecode block blocklist1 blocklist optblock
%type <std::unique_ptr<hadron::parse::ClassExtNode>> classextension classextensions
%type <std::unique_ptr<hadron::parse::ClassNode>> classdef classes
%type <std::unique_ptr<hadron::parse::ExprSeqNode>> dictslotlist1 dictslotdef arrayelems1
%type <std::unique_ptr<hadron::parse::ExprSeqNode>> exprseq methbody funcbody arglist1 arglistv1 dictslotlist arrayelems
%type <std::unique_ptr<hadron::parse::IfNode>> if
%type <std::unique_ptr<hadron::parse::KeyValueNode>> keyarg keyarglist1 optkeyarglist litdictslotdef litdictslotlist1
%type <std::unique_ptr<hadron::parse::KeyValueNode>> litdictslotlist
%type <std::unique_ptr<hadron::parse::LiteralDictNode>> dictlit2
%type <std::unique_ptr<hadron::parse::LiteralNode>> coreliteral
%type <std::unique_ptr<hadron::parse::LiteralListNode>> listlit listlit2
%type <std::unique_ptr<hadron::parse::MethodNode>> methods methoddef
%type <std::unique_ptr<hadron::parse::MultiAssignVarsNode>> mavars
%type <std::unique_ptr<hadron::parse::NameNode>> mavarlist
%type <std::unique_ptr<hadron::parse::Node>> root expr exprn expr1 /* adverb */ valrangex1 msgsend literallistc
%type <std::unique_ptr<hadron::parse::Node>> literallist1 literal listliteral
%type <std::unique_ptr<hadron::parse::ReturnNode>> funretval retval
%type <std::unique_ptr<hadron::parse::SeriesIterNode>> valrange3
%type <std::unique_ptr<hadron::parse::SeriesNode>> valrange2
%type <std::unique_ptr<hadron::parse::VarDefNode>> rwslotdeflist rwslotdef slotdef vardef constdef constdeflist
%type <std::unique_ptr<hadron::parse::VarDefNode>> vardeflist slotdeflist vardeflist0 slotdeflist0
%type <std::unique_ptr<hadron::parse::VarListNode>> classvardecl classvardecls funcvardecls funcvardecl funcvardecls1

%type <std::optional<size_t>> superclass optname
%type <std::optional<size_t>> primitive
%type <std::pair<bool, bool>> rwspec
%type <bool> rspec
%type <std::pair<size_t, int32_t>> integer
%type <std::pair<size_t, double>> float
%type <std::pair<size_t, hadron::Hash>> binop binop2

%precedence ASSIGN
%left BINOP KEYWORD PLUS MINUS ASTERISK LESSTHAN GREATERTHAN PIPE READWRITEVAR LEFTARROW
%precedence DOT
%start root

%{
#include "hadron/Parser.hpp"

#include "Keywords.hpp"
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

yy::parser::symbol_type yylex(hadron::Parser* hadronParser);
}

%%
root    : classes { hadronParser->addRoot(std::move($classes)); }
        | classextensions { hadronParser->addRoot(std::move($classextensions)); }
        | INTERPRET cmdlinecode { hadronParser->addRoot(std::move($cmdlinecode)); }
        ;

classes[target] : %empty { $target = nullptr; }
                | classes[build] classdef { $target = append(std::move($build), std::move($classdef)); }
                ;

classextensions[target] : classextension { $target = std::move($classextension); }
                        | classextensions[build] classextension {
                                $target = append(std::move($build), std::move($classextension));
                            }
                        ;

classdef    : CLASSNAME superclass OPENCURLY classvardecls methods CLOSECURLY {
                    auto classDef = std::make_unique<hadron::parse::ClassNode>($CLASSNAME.first);
                    classDef->superClassNameIndex = $superclass;
                    classDef->variables = std::move($classvardecls);
                    classDef->methods = std::move($methods);
                    $classdef = std::move(classDef);
                }
            | CLASSNAME OPENSQUARE optname CLOSESQUARE superclass OPENCURLY classvardecls methods CLOSECURLY {
                    auto classDef = std::make_unique<hadron::parse::ClassNode>($CLASSNAME.first);
                    classDef->superClassNameIndex = $superclass;
                    classDef->optionalNameIndex = $optname;
                    classDef->variables = std::move($classvardecls);
                    classDef->methods = std::move($methods);
                    $classdef = std::move(classDef);
                }
            ;

classextension  : PLUS CLASSNAME OPENCURLY methods CLOSECURLY {
                        auto classExt = std::make_unique<hadron::parse::ClassExtNode>($CLASSNAME.first);
                        classExt->methods = std::move($methods);
                        $classextension = std::move(classExt);
                    }
                ;

optname : %empty { $optname = std::nullopt; }
        | IDENTIFIER { $optname = $IDENTIFIER.first; }
        ;

superclass  : %empty { $superclass = std::nullopt; }
            | COLON CLASSNAME { $superclass = $CLASSNAME.first; }
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
                    auto method = std::make_unique<hadron::parse::MethodNode>($IDENTIFIER.first, false);
                    method->primitiveIndex = $primitive;
                    method->body = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                    method->body->arguments = std::move($argdecls);
                    method->body->variables = std::move($funcvardecls);
                    method->body->body = std::move($methbody);
                    $methoddef = std::move(method);
                }
            | ASTERISK IDENTIFIER OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY {
                    auto method = std::make_unique<hadron::parse::MethodNode>($IDENTIFIER.first, true);
                    method->primitiveIndex = $primitive;
                    method->body = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                    method->body->arguments = std::move($argdecls);
                    method->body->variables = std::move($funcvardecls);
                    method->body->body = std::move($methbody);
                    $methoddef = std::move(method);
                }
            | binop OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY {
                    auto method = std::make_unique<hadron::parse::MethodNode>($binop.first, false);
                    method->primitiveIndex = $primitive;
                    method->body = std::make_unique<hadron::parse::BlockNode>($OPENCURLY);
                    method->body->arguments = std::move($argdecls);
                    method->body->variables = std::move($funcvardecls);
                    method->body->body = std::move($methbody);
                    $methoddef = std::move(method);
                }
            | ASTERISK binop OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY {
                    auto method = std::make_unique<hadron::parse::MethodNode>($binop.first, true);
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

optcomma	: %empty
            | COMMA
            ;

optequal	: %empty
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
                performList->target = std::make_unique<hadron::parse::NameNode>($IDENTIFIER.first);
                performList->arguments = std::move($arglistv1);
                performList->keywordArguments = std::move($optkeyarglist);
                $msgsend = std::move(performList);
            }
        | CLASSNAME blocklist1 {
                // This is shorthand for Classname.new {args}
                auto newCall = std::make_unique<hadron::parse::NewNode>($CLASSNAME.first);
                newCall->target = std::make_unique<hadron::parse::NameNode>($CLASSNAME.first);
                newCall->arguments = std::move($blocklist1);
                $msgsend = std::move(newCall);
            }
        | CLASSNAME OPENPAREN CLOSEPAREN blocklist {
                // This is shorthand for Classname.new() {args}
                auto newCall = std::make_unique<hadron::parse::NewNode>($CLASSNAME.first);
                newCall->target = std::make_unique<hadron::parse::NameNode>($CLASSNAME.first);
                newCall->arguments = std::move($blocklist);
                $msgsend = std::move(newCall);
            }
        | CLASSNAME OPENPAREN keyarglist1 optcomma CLOSEPAREN blocklist {
                // This is shorthand for Classname.new(foo: bazz) {args}
                auto newCall = std::make_unique<hadron::parse::NewNode>($CLASSNAME.first);
                newCall->target = std::make_unique<hadron::parse::NameNode>($CLASSNAME.first);
                newCall->arguments = std::move($blocklist);
                newCall->keywordArguments = std::move($keyarglist1);
                $msgsend = std::move(newCall);
            }
        | CLASSNAME OPENPAREN arglist1 optkeyarglist CLOSEPAREN blocklist {
                // This is shorthand for Classname.new(arg, foo: bazz) {args}
                auto newCall = std::make_unique<hadron::parse::NewNode>($CLASSNAME.first);
                newCall->target = std::make_unique<hadron::parse::NameNode>($CLASSNAME.first);
                newCall->arguments = append<std::unique_ptr<hadron::parse::Node>>(std::move($arglist1),
                        std::move($blocklist));
                newCall->keywordArguments = std::move($optkeyarglist);
                $msgsend = std::move(newCall);
            }
        | CLASSNAME OPENPAREN arglistv1 optkeyarglist CLOSEPAREN {
                auto newCall = std::make_unique<hadron::parse::NewNode>($CLASSNAME.first);
                newCall->target = std::make_unique<hadron::parse::NameNode>($CLASSNAME.first);
                newCall->arguments = std::move($arglistv1);
                newCall->keywordArguments = std::move($optkeyarglist);
                $msgsend = std::move(newCall);
            }
        | expr DOT IDENTIFIER OPENPAREN keyarglist1 optcomma CLOSEPAREN blocklist {
                auto call = std::make_unique<hadron::parse::CallNode>($IDENTIFIER);
                call->target = std::move($expr);
                call->arguments = std::move($blocklist);
                call->keywordArguments = std::move($keyarglist1);
                $msgsend = std::move(call);
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
                auto performList = std::make_unique<hadron::parse::PerformListNode>($IDENTIFIER.first);
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
            ifNode->trueBlock = std::make_unique<hadron::parse::BlockNode>($true->tokenIndex);
            ifNode->trueBlock->body = std::move($true);
            ifNode->falseBlock = std::make_unique<hadron::parse::BlockNode>($false->tokenIndex);
            ifNode->falseBlock->body = std::move($false);
            $if = std::move(ifNode);
        }
    | IF OPENPAREN exprseq[condition] COMMA exprseq[true] optcomma CLOSEPAREN {
            auto ifNode = std::make_unique<hadron::parse::IfNode>($IF);
            ifNode->condition = std::move($condition);
            ifNode->trueBlock = std::make_unique<hadron::parse::BlockNode>($true->tokenIndex);
            ifNode->trueBlock->body = std::move($true);
            $if = std::move(ifNode);
        }
    | expr DOT IF OPENPAREN exprseq[true] COMMA exprseq[false] optcomma CLOSEPAREN {
            auto ifNode = std::make_unique<hadron::parse::IfNode>($IF);
            ifNode->condition = std::make_unique<hadron::parse::ExprSeqNode>($expr->tokenIndex, std::move($expr));
            ifNode->trueBlock = std::make_unique<hadron::parse::BlockNode>($true->tokenIndex);
            ifNode->trueBlock->body = std::move($true);
            ifNode->falseBlock = std::make_unique<hadron::parse::BlockNode>($false->tokenIndex);
            ifNode->falseBlock->body = std::move($false);
            $if = std::move(ifNode);
        }
    | expr DOT IF OPENPAREN exprseq[true] optcomma CLOSEPAREN {
            auto ifNode = std::make_unique<hadron::parse::IfNode>($IF);
            ifNode->condition = std::make_unique<hadron::parse::ExprSeqNode>($expr->tokenIndex, std::move($expr));
            ifNode->trueBlock = std::make_unique<hadron::parse::BlockNode>($true->tokenIndex);
            ifNode->trueBlock->body = std::move($true);
            $if = std::move(ifNode);
        }
    | IF OPENPAREN exprseq[condition] CLOSEPAREN block[true] optblock {
            auto ifNode = std::make_unique<hadron::parse::IfNode>($IF);
            ifNode->condition = std::move($condition);
            ifNode->trueBlock = std::move($true);
            if ($optblock) {
                ifNode->falseBlock = std::move($optblock);
            }
            $if = std::move(ifNode);
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
                            auto literal = std::make_unique<hadron::parse::LiteralNode>($KEYWORD.first,
                                hadron::Type::kSymbol, hadron::Slot());
                            auto exprSeq = std::make_unique<hadron::parse::ExprSeqNode>($KEYWORD.first,
                                std::move(literal));
                            $target = append(std::move(exprSeq), std::move($exprseq));
                        }
                    | arrayelems1[build] COMMA exprseq {
                            $target = append(std::move($build), std::move($exprseq));
                        }
                    | arrayelems1[build] COMMA KEYWORD exprseq {
                            auto literal = std::make_unique<hadron::parse::LiteralNode>($KEYWORD.first,
                                hadron::Type::kSymbol, hadron::Slot());
                            auto exprSeq = std::make_unique<hadron::parse::ExprSeqNode>($KEYWORD.first,
                                    std::move(literal));
                            auto affixes = append(std::move(exprSeq), std::move($exprseq));
                            $target = append(std::move($build), std::move(affixes));
                        }
                    | arrayelems1[build] COMMA exprseq[append] COLON exprseq[next] {
                        auto affixes = append(std::move($append), std::move($next));
                        $target = append(std::move($build), std::move(affixes));
                        }
                    ;

expr1[target]   : literal { $target = std::move($literal); }
                | IDENTIFIER { $target = std::make_unique<hadron::parse::NameNode>($IDENTIFIER.first); }
                | CURRYARGUMENT { $target = std::make_unique<hadron::parse::CurryArgumentNode>($CURRYARGUMENT); }
                | msgsend { $target = std::move($msgsend); }
                | OPENPAREN exprseq CLOSEPAREN {
                        // To keep consistent with variable-less cmdlinecode blocks that get parsed as this we point
                        // to the openening parenthesis with the tokenIndex.
                        $exprseq->tokenIndex = $OPENPAREN;
                        $target = std::move($exprseq);
                    }
                | TILDE IDENTIFIER {
                        auto name = std::make_unique<hadron::parse::NameNode>($IDENTIFIER.first);
                        name->isGlobal = true;
                        $target = std::move(name);
                    }
                | OPENSQUARE arrayelems CLOSESQUARE {
                        auto list = std::make_unique<hadron::parse::ListNode>($OPENSQUARE);
                        list->elements = std::move($arrayelems);
                        $target = std::move(list);
                    }
                | OPENPAREN valrange2 CLOSEPAREN { $target = std::move($valrange2); }
                | OPENPAREN COLON valrange3 CLOSEPAREN { $target = std::move($valrange3); }
                | OPENPAREN dictslotlist CLOSEPAREN {
                        auto dict = std::make_unique<hadron::parse::DictionaryNode>($OPENPAREN);
                        dict->elements = std::move($dictslotlist);
                        $target = std::move(dict);
                    }
                | expr1[build] OPENSQUARE arglist1 CLOSESQUARE {
                        auto arrayNode = std::make_unique<hadron::parse::ArrayReadNode>($OPENSQUARE);
                        arrayNode->targetArray = std::move($build);
                        arrayNode->indexArgument = std::move($arglist1);
                        $target = std::move(arrayNode);
                    }
                | valrangex1 { $target = std::move($valrangex1); }
                | if { $target = std::move($if); }
                ;

valrangex1  : expr1 OPENSQUARE arglist1 DOTDOT CLOSESQUARE {
                    auto copySeries = std::make_unique<hadron::parse::CopySeriesNode>($OPENSQUARE);
                    copySeries->target = std::move($expr1);
                    copySeries->first = std::move($arglist1);
                    $valrangex1 = std::move(copySeries);
                }
            | expr1 OPENSQUARE DOTDOT exprseq CLOSESQUARE {
                    auto copySeries = std::make_unique<hadron::parse::CopySeriesNode>($OPENSQUARE);
                    copySeries->target = std::move($expr1);
                    copySeries->last = std::move($exprseq);
                    $valrangex1 = std::move(copySeries);
                }
            | expr1 OPENSQUARE arglist1 DOTDOT exprseq CLOSESQUARE {
                    auto copySeries = std::make_unique<hadron::parse::CopySeriesNode>($OPENSQUARE);
                    copySeries->target = std::move($expr1);
                    copySeries->first = std::move($arglist1);
                    copySeries->last = std::move($exprseq);
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
                | CLASSNAME { $target = std::make_unique<hadron::parse::NameNode>($CLASSNAME.first); }
                | expr[build] binop2 /* adverb */ expr[rightHand] %prec BINOP {
                        auto binop = std::make_unique<hadron::parse::BinopCallNode>($binop2.first);
                        binop->leftHand = std::move($build);
                        binop->rightHand = std::move($rightHand);
                        /* binop->adverb = std::move($adverb); */
                        $target = std::move(binop);
                    }
                | IDENTIFIER ASSIGN expr[build] {
                        auto assign = std::make_unique<hadron::parse::AssignNode>($ASSIGN);
                        assign->name = std::make_unique<hadron::parse::NameNode>($IDENTIFIER.first);
                        assign->value = std::move($build);
                        $target = std::move(assign);
                    }
                | TILDE IDENTIFIER ASSIGN expr[build] {
                        auto assign = std::make_unique<hadron::parse::AssignNode>($ASSIGN);
                        assign->name = std::make_unique<hadron::parse::NameNode>($IDENTIFIER.first);
                        assign->name->isGlobal = true;
                        assign->value = std::move($build);
                        $target = std::move(assign);
                    }
                | expr[build] DOT IDENTIFIER ASSIGN expr[value] {
                        auto setter = std::make_unique<hadron::parse::SetterNode>($IDENTIFIER.first);
                        setter->target = std::move($build);
                        setter->value = std::move($value);
                        $target = std::move(setter);
                    }
                | IDENTIFIER OPENPAREN arglist1 optkeyarglist CLOSEPAREN ASSIGN expr[value] {
                        if ($optkeyarglist != nullptr) {
                            // TODO: LSC error condition "Setter method called with keyword arguments"
                        }
                        auto setter = std::make_unique<hadron::parse::SetterNode>($IDENTIFIER.first);
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
                    argList->varArgsNameIndex = $IDENTIFIER.first;
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
                    argList->varArgsNameIndex = $IDENTIFIER.first;
                    $argdecls = std::move(argList);
                }
            ;

constdeflist[target]    : constdef { $target = std::move($constdef); }
                        | constdeflist[build] optcomma constdef {
                                $target = append(std::move($build), std::move($constdef));
                            }
                        ;

constdef    : rspec IDENTIFIER ASSIGN literal {
                    auto constDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER.first);
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

slotdef : IDENTIFIER { $slotdef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER.first); }
        | IDENTIFIER optequal literal {
                auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER.first);
                varDef->initialValue = std::move($literal);
                $slotdef = std::move(varDef);
            }
        | IDENTIFIER optequal OPENPAREN exprseq CLOSEPAREN {
                auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER.first);
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

vardef  : IDENTIFIER { $vardef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER.first); }
        | IDENTIFIER ASSIGN expr {
                auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER.first);
                varDef->initialValue = std::move($expr);
                $vardef = std::move(varDef);
            }
        | IDENTIFIER OPENPAREN exprseq CLOSEPAREN {
                auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER.first);
                varDef->initialValue = std::move($exprseq);
                $vardef = std::move(varDef);
            }
        ;

dictslotdef  : exprseq[build] COLON exprseq[next] { $dictslotdef = append(std::move($build), std::move($next)); }
             | KEYWORD exprseq {
                    auto literal = std::make_unique<hadron::parse::LiteralNode>($KEYWORD.first,
                        hadron::Type::kSymbol, hadron::Slot());
                    auto exprSeq = std::make_unique<hadron::parse::ExprSeqNode>($KEYWORD.first, std::move(literal));
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
                        auto keyValue = std::make_unique<hadron::parse::KeyValueNode>($KEYWORD.first);
                        keyValue->key = std::make_unique<hadron::parse::NameNode>($KEYWORD.first);
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
                litList->className = std::make_unique<hadron::parse::NameNode>($CLASSNAME.first);
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
                    auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER.first);
                    varDef->hasReadAccessor = $rwspec.first;
                    varDef->hasWriteAccessor = $rwspec.second;
                    $rwslotdef = std::move(varDef);
                }
            | rwspec IDENTIFIER ASSIGN literal {
                    auto varDef = std::make_unique<hadron::parse::VarDefNode>($IDENTIFIER.first);
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
                auto keyArg = std::make_unique<hadron::parse::KeyValueNode>($KEYWORD.first);
                auto name = std::make_unique<hadron::parse::NameNode>($KEYWORD.first);
                keyArg->key = std::make_unique<hadron::parse::ExprSeqNode>($KEYWORD.first, std::move(name));
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
                multiVars->rest = std::make_unique<hadron::parse::NameNode>($IDENTIFIER.first);
                $mavars = std::move(multiVars);
            }
        ;

mavarlist[target]   : IDENTIFIER {
                            auto name = std::make_unique<hadron::parse::NameNode>($IDENTIFIER.first);
                            $target = std::move(name);
                        }
                    | mavarlist[build] COMMA IDENTIFIER {
                            auto name = std::make_unique<hadron::parse::NameNode>($IDENTIFIER.first);
                            $target = append(std::move($build), std::move(name));
                        }
                    ;

listliteral : coreliteral { $listliteral = std::move($coreliteral); }
        | listlit2 { $listliteral = std::move($listlit2); }
        | dictlit2 { $listliteral = std::move($dictlit2); }
        ;

literal : coreliteral { $literal = std::move($coreliteral); }
        | listlit { $literal = std::move($listlit); }
        ;

coreliteral : LITERAL {
                $coreliteral = std::make_unique<hadron::parse::LiteralNode>($LITERAL,
                    hadronParser->token($LITERAL).literalType, hadronParser->token($LITERAL).value);
                }
            | integer {
                    $coreliteral = std::make_unique<hadron::parse::LiteralNode>($integer.first,
                            hadron::Type::kInteger, hadron::Slot($integer.second));
                }
            | float {
                    $coreliteral = std::make_unique<hadron::parse::LiteralNode>($float.first,
                            hadron::Type::kFloat, hadron::Slot($float.second));
                }
            | block {
                    auto literal = std::make_unique<hadron::parse::LiteralNode>($block->tokenIndex,
                            hadron::Type::kBlock, hadron::Slot());
                    literal->blockLiteral = std::move($block);
                    $coreliteral = std::move(literal);
                }
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
        | PLUS { $binop = std::make_pair($PLUS, hadron::kAddHash); }
        | MINUS { $binop = std::make_pair($MINUS, hadron::kSubtractHash); }
        | ASTERISK { $binop = std::make_pair($ASTERISK, hadron::kMultiplyHash); }
        | LESSTHAN { $binop = std::make_pair($LESSTHAN, hadron::kLessThanHash); }
        | GREATERTHAN { $binop = std::make_pair($GREATERTHAN, hadron::kGreaterThanHash); }
        | PIPE { $binop = std::make_pair($PIPE, hadron::kPipeHash); }
        | READWRITEVAR { $binop = std::make_pair($READWRITEVAR, hadron::kReadWriteHash); }
        | LEFTARROW { $binop = std::make_pair($LEFTARROW, hadron::kLeftArrowHash); }
        ;

binop2  : binop { $binop2 = $binop; }
        | KEYWORD { $binop2 = $KEYWORD; }
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
        if (token.literalType == hadron::Type::kInteger) {
            return yy::parser::make_INTEGER(index, token.location);
        } else if (token.literalType == hadron::Type::kFloat) {
            return  yy::parser::make_FLOAT(index, token.location);
        }
        return yy::parser::make_LITERAL(index, token.location);

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
        return yy::parser::make_BINOP(std::make_pair(index, token.hash), token.location);

    case hadron::Token::Name::kKeyword:
        return yy::parser::make_KEYWORD(std::make_pair(index, token.hash), token.location);

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
        return yy::parser::make_IDENTIFIER(std::make_pair(index, token.hash), token.location);

    case hadron::Token::Name::kClassName:
        return yy::parser::make_CLASSNAME(std::make_pair(index, token.hash), token.location);

    case hadron::Token::Name::kDot:
        return yy::parser::make_DOT(index, token.location);

    case hadron::Token::Name::kDotDot:
        return yy::parser::make_DOTDOT(index, token.location);

    case hadron::Token::Name::kEllipses:
        return yy::parser::make_ELLIPSES(index, token.location);

    case hadron::Token::Name::kCurryArgument:
        return yy::parser::make_CURRYARGUMENT(index, token.location);

    case hadron::Token::Name::kIf:
        return yy::parser::make_IF(index, token.location);
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
    m_errorReporter(errorReporter) { }

Parser::Parser(std::string_view code):
    m_ownLexer(std::make_unique<Lexer>(code)),
    m_lexer(m_ownLexer.get()),
    m_tokenIndex(0),
    m_sendInterpret(false),
    m_errorReporter(m_ownLexer->errorReporter()) { }

Parser::~Parser() {}

bool Parser::parse() {
    m_sendInterpret = true;
    return innerParse();
}

bool Parser::parseClass() {
    m_sendInterpret = false;
    return innerParse();
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
        return Token();
    }
}

bool Parser::innerParse() {
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

    if (!m_root) {
        m_root = std::make_unique<hadron::parse::Node>(hadron::parse::NodeType::kEmpty, 0);
    }

    return m_errorReporter->errorCount() == 0;
}

} // namespace hadron