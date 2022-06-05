%require "3.2"
%language "c++"
%define api.value.type variant
%define api.token.constructor
%define parse.trace
%define parse.error verbose
%locations
%define api.location.type { hadron::Token::Location }
%param { hadron::Parser* hadronParser }
%param { hadron::ThreadContext* threadContext }
%skeleton "lalr1.cc"

%token END 0 "end of file"
%token INTERPRET
// Most tokens are just the index in the lexer's tokens() vector.
%token <hadron::library::Token> LITERAL FLOAT INTEGER PRIMITIVE PLUS MINUS ASTERISK ASSIGN LESSTHAN GREATERTHAN PIPE
%token <hadron::library::Token> LEFTARROW OPENPAREN CLOSEPAREN OPENCURLY CLOSECURLY OPENSQUARE CLOSESQUARE COMMA
%token <hadron::library::Token> READWRITEVAR SEMICOLON COLON CARET TILDE HASH GRAVE VAR ARG CONST CLASSVAR DOT DOTDOT
%token <hadron::library::Token> ELLIPSES CURRYARGUMENT IF WHILE STRING SYMBOL BINOP KEYWORD IDENTIFIER CLASSNAME
%token <hadron::library::Token> BEGINCLOSEDFUNC PI

%type <hadron::library::ArgListNode> argdecls
%type <hadron::library::BlockNode> cmdlinecode block blockliteral optblock
%type <hadron::library::ClassExtNode> classextension
%type <hadron::library::ClassNode> classdef
%type <hadron::library::ExprSeqNode> dictslotlist1 dictslotdef arrayelems1
%type <hadron::library::ExprSeqNode> exprseq methbody funcbody arglist1 arglistv1 dictslotlist arrayelems
%type <hadron::library::IfNode> if
%type <hadron::library::WhileNode> while
%type <hadron::library::KeyValueNode> keyarg keyarglist1 optkeyarglist litdictslotdef litdictslotlist1
%type <hadron::library::KeyValueNode> litdictslotlist
%type <hadron::library::EventNode> dictlit2
%type <hadron::library::CollectionNode> listlit listlit2
%type <hadron::library::ListCompNode> listcomp
%type <hadron::library::MethodNode> methods methoddef
%type <hadron::library::MultiAssignVarsNode> mavars
%type <hadron::library::NameNode> mavarlist
%type <hadron::library::Node> root expr exprn expr1 adverb valrangex1 msgsend literallistc valrangeassign
%type <hadron::library::Node> literallist1 literal listliteral coreliteral classorclassext
%type <hadron::library::Node> classorclassexts qualifiers qual blocklistitem blocklist1 blocklist
%type <hadron::library::ReturnNode> funretval retval
%type <hadron::library::SeriesIterNode> valrange3
%type <hadron::library::SeriesNode> valrange2
%type <hadron::library::VarDefNode> rwslotdeflist rwslotdef slotdef vardef constdef constdeflist
%type <hadron::library::VarDefNode> vardeflist slotdeflist vardeflist0 slotdeflist0
%type <hadron::library::VarListNode> classvardecl classvardecls funcvardecls funcvardecl funcvardecls1
%type <hadron::library::StringNode> string stringlist

%type <hadron::library::Token> superclass optname
%type <hadron::library::Token> primitive
%type <std::pair<bool, bool>> rwspec
%type <bool> rspec
%type <std::pair<hadron::library::Token, int32_t>> integer
%type <std::pair<hadron::library::Token, double>> floatr float
%type <hadron::library::Token> binop binop2

%precedence ASSIGN
%left BINOP KEYWORD PLUS MINUS ASTERISK LESSTHAN GREATERTHAN PIPE READWRITEVAR
%precedence DOT
%start root

%{
#include "hadron/Parser.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Hash.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"
#include "hadron/Token.hpp"

#include <cmath>

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
        head.append(tail.toBase());
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
hadron::library::BlockNode wrapInnerBlock(hadron::ThreadContext* threadContext, hadron::library::ExprSeqNode exprSeq) {
    if (exprSeq.expr().className() == hadron::library::BlockNode::nameHash()) {
        return hadron::library::BlockNode(exprSeq.expr().slot());
    }

    auto block = hadron::library::BlockNode::make(threadContext, exprSeq.token());
    block.setBody(exprSeq);
    return block;
}

yy::parser::symbol_type yylex(hadron::Parser* hadronParser, hadron::ThreadContext* threadContext);

// If first has a next, removes it and returns it. Otherwise returns an empty Node.
inline hadron::library::Node removeTail(hadron::library::Node head) {
    auto tail = hadron::library::Node::wrapUnsafe(head.next().slot());
    if (tail) {
        head.setNext(hadron::library::Node());
        tail.setTail(head.tail());
        head.setTail(head);
    }
    return tail;
}
}

%%
root    : classorclassexts { hadronParser->addRoot($classorclassexts); }
        | INTERPRET cmdlinecode { hadronParser->addRoot($cmdlinecode.toBase()); }
        ;

classorclassexts[target]    : %empty { $target = hadron::library::Node(); }
                            | classorclassexts[build] classorclassext {
                                    $target = append($build, $classorclassext);
                                }
                            ;

classorclassext : classdef { $classorclassext = $classdef.toBase(); }
                | classextension { $classorclassext = $classextension.toBase(); }
                ;

classdef    : CLASSNAME superclass OPENCURLY classvardecls methods CLOSECURLY {
                    auto classDef = hadron::library::ClassNode::make(threadContext, $CLASSNAME);
                    classDef.setSuperclassNameToken($superclass);
                    classDef.setVariables($classvardecls);
                    classDef.setMethods($methods);
                    $classdef = classDef;
                }
            | CLASSNAME OPENSQUARE optname CLOSESQUARE superclass OPENCURLY classvardecls methods CLOSECURLY {
                    auto classDef = hadron::library::ClassNode::make(threadContext, $CLASSNAME);
                    classDef.setSuperclassNameToken($superclass);
                    classDef.setOptionalNameToken($optname);
                    classDef.setVariables($classvardecls);
                    classDef.setMethods($methods);
                    $classdef = classDef;
                }
            ;

classextension  : PLUS CLASSNAME OPENCURLY methods CLOSECURLY {
                        auto classExt = hadron::library::ClassExtNode::make(threadContext, $CLASSNAME);
                        classExt.setMethods($methods);
                        $classextension = classExt;
                    }
                ;

optname : %empty { $optname = hadron::library::Token(); }
        | IDENTIFIER { $optname = $IDENTIFIER; }
        ;

superclass  : %empty { $superclass = hadron::library::Token(); }
            | COLON CLASSNAME { $superclass = $CLASSNAME; }
            ;

classvardecls[target]   : %empty { $target = hadron::library::VarListNode(); }
                        | classvardecls[build] classvardecl {
                               $target = append($build, $classvardecl);
                            }
                        ;

classvardecl    : CLASSVAR rwslotdeflist SEMICOLON {
                        auto varList = hadron::library::VarListNode::make(threadContext, $CLASSVAR);
                        varList.setDefinitions($rwslotdeflist);
                        $classvardecl = varList;
                    }
                | VAR rwslotdeflist SEMICOLON {
                        auto varList = hadron::library::VarListNode::make(threadContext, $VAR);
                        varList.setDefinitions($rwslotdeflist);
                        $classvardecl = varList;
                    }
                | CONST constdeflist SEMICOLON {
                        auto varList = hadron::library::VarListNode::make(threadContext, $CONST);
                        varList.setDefinitions($constdeflist);
                        $classvardecl = varList;
                    }
                ;

methods[target] : %empty { $target = hadron::library::MethodNode(); }
                | methods[build] methoddef { $target = append($build, $methoddef); }
                ;

methoddef   : IDENTIFIER OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY {
                    auto method = hadron::library::MethodNode::make(threadContext, $IDENTIFIER);
                    method.setIsClassMethod(false);
                    method.setPrimitiveToken($primitive);
                    auto block = hadron::library::BlockNode::make(threadContext, $OPENCURLY);
                    block.setArguments($argdecls);
                    block.setVariables($funcvardecls);
                    block.setBody($methbody);
                    method.setBody(block);
                    $methoddef = method;
                }
            | ASTERISK IDENTIFIER OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY {
                    auto method = hadron::library::MethodNode::make(threadContext, $IDENTIFIER);
                    method.setIsClassMethod(true);
                    method.setPrimitiveToken($primitive);
                    auto block = hadron::library::BlockNode::make(threadContext, $OPENCURLY);
                    block.setArguments($argdecls);
                    block.setVariables($funcvardecls);
                    block.setBody($methbody);
                    method.setBody(block);
                    $methoddef = method;
                }
            | binop OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY {
                    auto method = hadron::library::MethodNode::make(threadContext, $binop);
                    method.setIsClassMethod(false);
                    method.setPrimitiveToken($primitive);
                    auto block = hadron::library::BlockNode::make(threadContext, $OPENCURLY);
                    block.setArguments($argdecls);
                    block.setVariables($funcvardecls);
                    block.setBody($methbody);
                    method.setBody(block);
                    $methoddef = method;
                }
            | ASTERISK binop OPENCURLY argdecls funcvardecls primitive methbody CLOSECURLY {
                    auto method = hadron::library::MethodNode::make(threadContext, $binop);
                    method.setIsClassMethod(true);
                    method.setPrimitiveToken($primitive);
                    auto block = hadron::library::BlockNode::make(threadContext, $OPENCURLY);
                    block.setArguments($argdecls);
                    block.setVariables($funcvardecls);
                    block.setBody($methbody);
                    method.setBody(block);
                    $methoddef = method;
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

optblock    : %empty { $optblock = hadron::library::BlockNode(); }
            | block { $optblock = $block; }
            ;

funcbody    : funretval {
                    if ($funretval) {
                        auto exprSeq = hadron::library::ExprSeqNode::make(threadContext, $funretval.token());
                        exprSeq.setExpr($funretval.toBase());
                        $funcbody = exprSeq;
                    } else {
                        $funcbody = hadron::library::ExprSeqNode();
                    }
                }
            | exprseq funretval {
                    if ($funretval) {
                        $exprseq.expr().append($funretval.toBase());
                    }
                    $funcbody = $exprseq;
                }
            ;


cmdlinecode : OPENPAREN funcvardecls1 funcbody CLOSEPAREN {
                    auto block = hadron::library::BlockNode::make(threadContext, $OPENPAREN);
                    block.setVariables($funcvardecls1);
                    block.setBody($funcbody);
                    $cmdlinecode = block;
                }
            | funcvardecls1 funcbody {
                    auto block = hadron::library::BlockNode::make(threadContext, $funcvardecls1.token());
                    block.setVariables($funcvardecls1);
                    block.setBody($funcbody);
                    $cmdlinecode = block;
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
                        auto block = hadron::library::BlockNode::make(threadContext, $funcbody.token());
                        if ($funcbody.expr().className() == hadron::library::ExprSeqNode::nameHash()) {
                            auto exprSeq = hadron::library::ExprSeqNode($funcbody.expr().slot());
                            block.setBody(exprSeq);
                        } else {
                            block.setBody($funcbody);
                        }
                        $cmdlinecode = block;
                    } else {
                        $cmdlinecode = hadron::library::BlockNode();
                    }
                }
            ;

// TODO: why not merge with funcbody?
methbody    : retval {
                    if ($retval) {
                        auto exprSeq = hadron::library::ExprSeqNode::make(threadContext, $retval.token());
                        exprSeq.setExpr($retval.toBase());
                        $methbody = exprSeq;
                    } else {
                        $methbody = hadron::library::ExprSeqNode();
                    }
                }
            | exprseq retval {
                    if ($retval) {
                        $exprseq.expr().append($retval.toBase());
                    }
                    $methbody = $exprseq;
                }
            ;

primitive   : %empty { $primitive = hadron::library::Token(); }
            | PRIMITIVE optsemi { $primitive = $PRIMITIVE; }
            ;

retval  : %empty { $retval = hadron::library::ReturnNode(); }
        | CARET expr optsemi {
                auto ret = hadron::library::ReturnNode::make(threadContext, $CARET);
                ret.setValueExpr($expr);
                $retval = ret;
            }
        ;

funretval   : %empty { $funretval = hadron::library::ReturnNode(); }
            | CARET expr optsemi {
                    auto ret = hadron::library::ReturnNode::make(threadContext, $CARET);
                    ret.setValueExpr($expr);
                    $funretval = ret;
                }
            ;

blocklist1[target]  : blocklistitem { $target = $blocklistitem; }
                    | blocklist1[build] blocklistitem { $target = append($build, $blocklistitem); }
                    ;

blocklistitem   : blockliteral { $blocklistitem = $blockliteral.toBase(); }
                | listcomp { $blocklistitem = $listcomp.toBase(); }
                ;

blocklist   : %empty { $blocklist = hadron::library::Node(); }
            | blocklist1 { $blocklist = $blocklist1; }
            ;

msgsend : IDENTIFIER blocklist1 {
                auto call = hadron::library::CallNode::make(threadContext, $IDENTIFIER);
                call.setArguments($blocklist1.toBase());
                $msgsend = call.toBase();
            }
        | OPENPAREN binop2 CLOSEPAREN blocklist1 {
                auto call = hadron::library::CallNode::make(threadContext, $binop2);
                call.setArguments($blocklist1.toBase());
                $msgsend = call.toBase();
            }
        | IDENTIFIER OPENPAREN CLOSEPAREN blocklist1 {
                auto call = hadron::library::CallNode::make(threadContext, $IDENTIFIER);
                call.setArguments($blocklist1.toBase());
                $msgsend = call.toBase();
            }
        | IDENTIFIER OPENPAREN arglist1 optkeyarglist CLOSEPAREN blocklist {
                auto call = hadron::library::CallNode::make(threadContext, $IDENTIFIER);
                call.setArguments(append($arglist1.toBase(), $blocklist.toBase()));
                call.setKeywordArguments($optkeyarglist);
                $msgsend = call.toBase();
            }
        | OPENPAREN binop2 CLOSEPAREN OPENPAREN CLOSEPAREN blocklist1 {
                auto call = hadron::library::CallNode::make(threadContext, $binop2);
                call.setArguments($blocklist1);
                $msgsend = call.toBase();
            }
        | OPENPAREN binop2 CLOSEPAREN OPENPAREN arglist1 optkeyarglist CLOSEPAREN blocklist {
                auto call = hadron::library::CallNode::make(threadContext, $binop2);
                call.setArguments(append($arglist1.toBase(), $blocklist.toBase()));
                call.setKeywordArguments($optkeyarglist);
                $msgsend = call.toBase();
            }
        | IDENTIFIER OPENPAREN arglistv1 optkeyarglist CLOSEPAREN {
                // TODO - differentiate between this and superPerformList()
                auto performList = hadron::library::PerformListNode::make(threadContext, $OPENPAREN);
                performList.setTarget(hadron::library::NameNode::make(threadContext, $IDENTIFIER).toBase());
                performList.setArguments($arglistv1.toBase());
                performList.setKeywordArguments($optkeyarglist);
                $msgsend = performList.toBase();
            }
        | OPENPAREN[first] binop2 CLOSEPAREN OPENPAREN[second] arglistv1 optkeyarglist CLOSEPAREN {
                auto performList = hadron::library::PerformListNode::make(threadContext, $first);
                performList.setTarget(hadron::library::NameNode::make(threadContext, $binop2).toBase());
                performList.setArguments($arglistv1.toBase());
                performList.setKeywordArguments($optkeyarglist);
                $msgsend = performList.toBase();
            }
        | CLASSNAME OPENSQUARE arrayelems CLOSESQUARE {
                // This is shorthand for Classname.new() followed by (args.add)
                auto collection = hadron::library::CollectionNode::make(threadContext, $OPENSQUARE);
                collection.setClassName(hadron::library::NameNode::make(threadContext, $CLASSNAME));
                collection.setElements($arrayelems.toBase());
                $msgsend = collection.toBase();
            }
        | CLASSNAME blocklist1 {
                // This is shorthand for Classname.new {args}
                auto newCall = hadron::library::NewNode::make(threadContext, $CLASSNAME);
                newCall.setTarget(hadron::library::NameNode::make(threadContext, $CLASSNAME).toBase());
                newCall.setArguments($blocklist1.toBase());
                $msgsend = newCall.toBase();
            }
        | CLASSNAME OPENPAREN CLOSEPAREN blocklist {
                // This is shorthand for Classname.new() {args}
                auto newCall = hadron::library::NewNode::make(threadContext, $CLASSNAME);
                newCall.setTarget(hadron::library::NameNode::make(threadContext, $CLASSNAME).toBase());
                newCall.setArguments($blocklist.toBase());
                $msgsend = newCall.toBase();
            }
        | CLASSNAME OPENPAREN keyarglist1 optcomma CLOSEPAREN blocklist {
                // This is shorthand for Classname.new(foo: bazz) {args}
                auto newCall = hadron::library::NewNode::make(threadContext, $CLASSNAME);
                newCall.setTarget(hadron::library::NameNode::make(threadContext, $CLASSNAME).toBase());
                newCall.setArguments($blocklist.toBase());
                newCall.setKeywordArguments($keyarglist1);
                $msgsend = newCall.toBase();
            }
        | CLASSNAME OPENPAREN arglist1 optkeyarglist CLOSEPAREN blocklist {
                // This is shorthand for Classname.new(arg, foo: bazz) {args}
                auto newCall = hadron::library::NewNode::make(threadContext, $CLASSNAME);
                newCall.setTarget(hadron::library::NameNode::make(threadContext, $CLASSNAME).toBase());
                newCall.setArguments(append<hadron::library::Node>($arglist1.toBase(), $blocklist.toBase()));
                newCall.setKeywordArguments($optkeyarglist);
                $msgsend = newCall.toBase();
            }
        | CLASSNAME OPENPAREN arglistv1 optkeyarglist CLOSEPAREN {
                auto performList = hadron::library::PerformListNode::make(threadContext, $OPENPAREN);
                auto newCall = hadron::library::NewNode::make(threadContext, $CLASSNAME);
                newCall.setTarget(hadron::library::NameNode::make(threadContext, $CLASSNAME).toBase());
                performList.setArguments(append(newCall.toBase(), $arglistv1.toBase()));
                performList.setKeywordArguments($optkeyarglist);
                $msgsend = newCall.toBase();
            }
        | expr DOT OPENPAREN CLOSEPAREN blocklist {
                auto value = hadron::library::ValueNode::make(threadContext, $DOT);
                value.setTarget($expr);
                value.setArguments($blocklist.toBase());
                $msgsend = value.toBase();
            }
        | expr DOT OPENPAREN keyarglist1 optcomma CLOSEPAREN blocklist {
                auto value = hadron::library::ValueNode::make(threadContext, $DOT);
                value.setTarget($expr);
                value.setArguments($blocklist.toBase());
                value.setKeywordArguments($keyarglist1);
                $msgsend = value.toBase();
            }
        | expr DOT IDENTIFIER OPENPAREN keyarglist1 optcomma CLOSEPAREN blocklist {
                auto call = hadron::library::CallNode::make(threadContext, $IDENTIFIER);
                call.setTarget($expr);
                call.setArguments($blocklist.toBase());
                call.setKeywordArguments($keyarglist1);
                $msgsend = call.toBase();
            }
        | expr DOT OPENPAREN arglist1 optkeyarglist CLOSEPAREN blocklist {
                auto value = hadron::library::ValueNode::make(threadContext, $DOT);
                value.setTarget($expr);
                value.setArguments(append<hadron::library::Node>($arglist1.toBase(), $blocklist.toBase()));
                value.setKeywordArguments($optkeyarglist);
                $msgsend = value.toBase();
            }
        | expr DOT OPENPAREN arglistv1 optkeyarglist CLOSEPAREN {
                auto performList = hadron::library::PerformListNode::make(threadContext, $OPENPAREN);
                auto valueCall = hadron::library::ValueNode::make(threadContext, $DOT);
                valueCall.setTarget($expr);
                performList.setArguments(append(valueCall.toBase(), $arglistv1.toBase()));
                performList.setKeywordArguments($optkeyarglist);
                $msgsend = performList.toBase();
            }
        | expr DOT IDENTIFIER OPENPAREN CLOSEPAREN blocklist {
                auto call = hadron::library::CallNode::make(threadContext, $IDENTIFIER);
                call.setTarget($expr);
                call.setArguments($blocklist.toBase());
                $msgsend = call.toBase();
            }
        | expr DOT IDENTIFIER OPENPAREN arglist1 optkeyarglist CLOSEPAREN blocklist {
                auto call = hadron::library::CallNode::make(threadContext, $IDENTIFIER);
                call.setTarget($expr);
                call.setArguments(append<hadron::library::Node>($arglist1.toBase(), $blocklist.toBase()));
                call.setKeywordArguments($optkeyarglist);
                $msgsend = call.toBase();
            }
        | expr DOT IDENTIFIER OPENPAREN arglistv1 optkeyarglist CLOSEPAREN {
                // TODO - differentiate between this and superPerformList()
                auto performList = hadron::library::PerformListNode::make(threadContext, $OPENPAREN);
                performList.setTarget($expr);
                performList.setArguments($arglistv1.toBase());
                performList.setKeywordArguments($optkeyarglist);
                $msgsend = performList.toBase();
            }
        | expr DOT IDENTIFIER blocklist {
                auto call = hadron::library::CallNode::make(threadContext, $IDENTIFIER);
                call.setTarget($expr);
                call.setArguments($blocklist.toBase());
                $msgsend = call.toBase();
            }
        ;

listcomp: OPENCURLY COLON exprseq COMMA qualifiers CLOSECURLY {
                auto listComp = hadron::library::ListCompNode::make(threadContext, $OPENCURLY);
                listComp.setBody($exprseq);
                listComp.setQualifiers($qualifiers);
                $listcomp = listComp;
            }
        | OPENCURLY SEMICOLON exprseq COMMA qualifiers CLOSECURLY {
                auto listComp = hadron::library::ListCompNode::make(threadContext, $OPENCURLY);
                listComp.setBody($exprseq);
                listComp.setQualifiers($qualifiers);
                $listcomp = listComp;
            }
        ;

qualifiers[target]  : qual { $target = $qual; }
                    | qualifiers[build] COMMA qual { $target = append($build, $qual); }
                    ;

qual: IDENTIFIER LEFTARROW exprseq {
            auto generator = hadron::library::GenQualNode::make(threadContext, $LEFTARROW);
            generator.setName(hadron::library::NameNode::make(threadContext, $IDENTIFIER));
            generator.setExprSeq($exprseq);
            $qual = generator.toBase();
        }
    | exprseq {
            auto guard = hadron::library::GuardQualNode::make(threadContext, $exprseq.token());
            guard.setExprSeq($exprseq);
            $qual = guard.toBase();
        }
    | VAR IDENTIFIER ASSIGN exprseq {
            auto binding = hadron::library::BindingQualNode::make(threadContext, $VAR);
            binding.setName(hadron::library::NameNode::make(threadContext, $IDENTIFIER));
            binding.setExprSeq($exprseq);
            $qual = binding.toBase();
        }
    | COLON[first] COLON[second] exprseq {
            auto sideEffect = hadron::library::SideEffectQualNode::make(threadContext, $first);
            sideEffect.setExprSeq($exprseq);
            $qual = sideEffect.toBase();
        }
    | COLON WHILE exprseq {
            auto termination = hadron::library::TerminationQualNode::make(threadContext, $COLON);
            termination.setExprSeq($exprseq);
            $qual = termination.toBase();
        }
    ;

if  : IF OPENPAREN exprseq[condition] COMMA exprseq[true] COMMA exprseq[false] optcomma CLOSEPAREN {
            auto ifNode = hadron::library::IfNode::make(threadContext, $IF);
            ifNode.setCondition($condition);
            ifNode.setTrueBlock(wrapInnerBlock(threadContext, $true));
            ifNode.setElseBlock(wrapInnerBlock(threadContext, $false));
            $if = ifNode;
        }
    | IF OPENPAREN exprseq[condition] COMMA exprseq[true] optcomma CLOSEPAREN {
            auto ifNode = hadron::library::IfNode::make(threadContext, $IF);
            ifNode.setCondition($condition);
            ifNode.setTrueBlock(wrapInnerBlock(threadContext, $true));
            $if = ifNode;
        }
    | expr DOT IF OPENPAREN exprseq[true] COMMA exprseq[false] optcomma CLOSEPAREN {
            auto ifNode = hadron::library::IfNode::make(threadContext, $IF);
            auto condSeq = hadron::library::ExprSeqNode::make(threadContext, $expr.token());
            condSeq.setExpr($expr);
            ifNode.setCondition(condSeq);
            ifNode.setTrueBlock(wrapInnerBlock(threadContext, $true));
            ifNode.setElseBlock(wrapInnerBlock(threadContext, $false));
            $if = ifNode;
        }
    | expr DOT IF OPENPAREN exprseq[true] optcomma CLOSEPAREN {
            auto ifNode = hadron::library::IfNode::make(threadContext, $IF);
            auto condSeq = hadron::library::ExprSeqNode::make(threadContext, $expr.token());
            condSeq.setExpr($expr);
            ifNode.setCondition(condSeq);
            ifNode.setTrueBlock(wrapInnerBlock(threadContext, $true));
            $if = ifNode;
        }
    | expr DOT IF block[true] optblock {
            auto ifNode = hadron::library::IfNode::make(threadContext, $IF);
            auto condSeq = hadron::library::ExprSeqNode::make(threadContext, $expr.token());
            condSeq.setExpr($expr);
            ifNode.setCondition(condSeq);
            ifNode.setTrueBlock($true);
            ifNode.setElseBlock($optblock);
            $if = ifNode;
        }
    | IF OPENPAREN exprseq[condition] CLOSEPAREN block[true] optblock {
            auto ifNode = hadron::library::IfNode::make(threadContext, $IF);
            ifNode.setCondition($condition);
            ifNode.setTrueBlock($true);
            ifNode.setElseBlock($optblock);
            $if = ifNode;
        }
    ;

while   : WHILE OPENPAREN block[condition] optcomma blocklist[blocks] CLOSEPAREN {
                auto whileNode = hadron::library::WhileNode::make(threadContext, $WHILE);
                whileNode.setConditionBlock($condition);
                whileNode.setActionBlock(hadron::library::BlockNode($blocks.slot()));
                $while = whileNode;
            }
        | WHILE blocklist1[blocks] {
                auto whileNode = hadron::library::WhileNode::make(threadContext, $WHILE);
                // Extract second block as condition block, if present.
                auto actionBlock = hadron::library::BlockNode(removeTail($blocks.toBase()).slot());
                whileNode.setConditionBlock(hadron::library::BlockNode($blocks.slot()));
                whileNode.setActionBlock(actionBlock);
                $while = whileNode;
            }
        ;


adverb  : %empty { $adverb = hadron::library::Node(); }
        | DOT IDENTIFIER { $adverb = hadron::library::SymbolNode::make(threadContext, $IDENTIFIER).toBase(); }
        | DOT INTEGER {
                auto literal = hadron::library::SlotNode::make(threadContext, $INTEGER);
                literal.setValue($INTEGER.value());
                $adverb = literal.toBase();
            }
        | DOT OPENPAREN exprseq CLOSEPAREN { $adverb = $exprseq.toBase(); }
        ;

exprn[target]   : expr { $target = $expr; }
                | exprn[build] SEMICOLON expr { $target = append($build, $expr); }
                ;

exprseq : exprn optsemi {
                auto exprSeq = hadron::library::ExprSeqNode::make(threadContext, $exprn.token());
                exprSeq.setExpr($exprn);
                $exprseq = exprSeq;
            }
        ;

arrayelems  : %empty { $arrayelems = hadron::library::ExprSeqNode(); }
            | arrayelems1 optcomma { $arrayelems = $arrayelems1; }
            ;

arrayelems1[target] : exprseq { $target = $exprseq; }
                    | exprseq[build] COLON exprseq[next] { $target = append($build, $next); }
                    | KEYWORD exprseq {
                            auto symbol = hadron::library::SymbolNode::make(threadContext, $KEYWORD);
                            auto exprSeq = hadron::library::ExprSeqNode::make(threadContext, $KEYWORD);
                            exprSeq.setExpr(symbol.toBase());
                            $target = append(exprSeq, $exprseq);
                        }
                    | arrayelems1[build] COMMA exprseq {
                            $target = append($build, $exprseq);
                        }
                    | arrayelems1[build] COMMA KEYWORD exprseq {
                            auto symbol = hadron::library::SymbolNode::make(threadContext, $KEYWORD);
                            auto exprSeq = hadron::library::ExprSeqNode::make(threadContext, $KEYWORD);
                            exprSeq.setExpr(symbol.toBase());
                            auto affixes = append(exprSeq, $exprseq);
                            $target = append($build, affixes);
                        }
                    | arrayelems1[build] COMMA exprseq[append] COLON exprseq[next] {
                            auto affixes = append($append, $next);
                            $target = append($build, affixes);
                        }
                    ;

expr1[target]   : literal { $target = $literal; }
                | blockliteral { $target = $blockliteral.toBase(); }
                | listcomp { $target = $listcomp.toBase(); }
                | IDENTIFIER { $target = hadron::library::NameNode::make(threadContext, $IDENTIFIER).toBase(); }
                | CURRYARGUMENT {
                        $target = hadron::library::CurryArgumentNode::make(threadContext, $CURRYARGUMENT).toBase();
                    }
                | msgsend { $target = $msgsend.toBase(); }
                | OPENPAREN exprseq CLOSEPAREN {
                        // To keep consistent with variable-less cmdlinecode blocks that get parsed as this we point
                        // to the openening parenthesis with the tokenIndex.
                        $exprseq.setToken($OPENPAREN);
                        $target = $exprseq.toBase();
                    }
                | TILDE IDENTIFIER {
                        auto envirAt = hadron::library::EnvironmentAtNode::make(threadContext, $IDENTIFIER);
                        $target = envirAt.toBase();
                    }
                | OPENSQUARE arrayelems CLOSESQUARE {
                        auto array = hadron::library::CollectionNode::make(threadContext, $OPENSQUARE);
                        array.setElements($arrayelems.toBase());
                        $target = array.toBase();
                    }
                | OPENPAREN valrange2 CLOSEPAREN { $target = $valrange2.toBase(); }
                | OPENPAREN COLON valrange3 CLOSEPAREN { $target = $valrange3.toBase(); }
                | OPENPAREN dictslotlist CLOSEPAREN {
                        auto event = hadron::library::EventNode::make(threadContext, $OPENPAREN);
                        event.setElements($dictslotlist.toBase());
                        $target = event.toBase();
                    }
                | expr1[build] OPENSQUARE arglist1 CLOSESQUARE {
                        auto arrayNode = hadron::library::ArrayReadNode::make(threadContext, $OPENSQUARE);
                        arrayNode.setTargetArray($build);
                        arrayNode.setIndexArgument($arglist1);
                        $target = arrayNode.toBase();
                    }
                | valrangex1 { $target = $valrangex1; }
                | if { $target = $if.toBase(); }
                | while { $target = $while.toBase(); }
                ;

valrangex1  : expr1 OPENSQUARE arglist1 DOTDOT CLOSESQUARE {
                    auto copySeries = hadron::library::CopySeriesNode::make(threadContext, $OPENSQUARE);
                    copySeries.setTarget($expr1);
                    auto first = $arglist1;
                    auto second = removeTail(first.toBase());
                    copySeries.setFirst(first);
                    copySeries.setSecond(second);
                    $valrangex1 = copySeries.toBase();
                }
            | expr1 OPENSQUARE DOTDOT exprseq[last] CLOSESQUARE {
                    auto copySeries = hadron::library::CopySeriesNode::make(threadContext, $OPENSQUARE);
                    copySeries.setTarget($expr1);
                    copySeries.setLast($last);
                    $valrangex1 = copySeries.toBase();
                }
            | expr1 OPENSQUARE arglist1 DOTDOT exprseq[last] CLOSESQUARE {
                    auto copySeries = hadron::library::CopySeriesNode::make(threadContext, $OPENSQUARE);
                    copySeries.setTarget($expr1);
                    auto first = $arglist1;
                    auto second = removeTail(first.toBase());
                    copySeries.setFirst(first);
                    copySeries.setSecond(second);
                    copySeries.setLast($last);
                    $valrangex1 = copySeries.toBase();
                }
            ;

valrangeassign  : expr1 OPENSQUARE arglist1 DOTDOT CLOSESQUARE ASSIGN expr {
                        auto putSeries = hadron::library::PutSeriesNode::make(threadContext, $ASSIGN);
                        putSeries.setTarget($expr1);
                        auto first = $arglist1.toBase();
                        auto second = removeTail(first);
                        putSeries.setFirst(first);
                        putSeries.setSecond(second);
                        putSeries.setValue($expr);
                        $valrangeassign = putSeries.toBase();
                    }
                | expr1 OPENSQUARE DOTDOT exprseq CLOSESQUARE ASSIGN expr {
                        auto putSeries = hadron::library::PutSeriesNode::make(threadContext, $ASSIGN);
                        putSeries.setTarget($expr1);
                        putSeries.setLast($exprseq.toBase());
                        putSeries.setValue($expr);
                        $valrangeassign = putSeries.toBase();
                    }
                | expr1 OPENSQUARE arglist1 DOTDOT exprseq CLOSESQUARE ASSIGN expr {
                        auto putSeries = hadron::library::PutSeriesNode::make(threadContext, $ASSIGN);
                        putSeries.setTarget($expr1);
                        auto first = $arglist1.toBase();
                        auto second = removeTail(first);
                        putSeries.setFirst(first);
                        putSeries.setSecond(second);
                        putSeries.setLast($exprseq.toBase());
                        putSeries.setValue($expr);
                        $valrangeassign = putSeries.toBase();
                    }
                ;

// (start, step..size) --> SimpleNumber.series(start, step, last) -> start.series(step, last)
valrange2   : exprseq[start] DOTDOT {
                    // There are only certain contexts that this is a valid construction, a 'do' operation and a list
                    // comprehension. (Where the last value is implied)
                    auto series = hadron::library::SeriesNode::make(threadContext, $DOTDOT);
                    series.setStart($start);
                    $valrange2 = series;
                }
            | DOTDOT exprseq[last] {
                    auto series = hadron::library::SeriesNode::make(threadContext, $DOTDOT);
                    series.setLast($last);
                    $valrange2 = series;
                }
            | exprseq[start] DOTDOT exprseq[last] {
                    auto series = hadron::library::SeriesNode::make(threadContext, $DOTDOT);
                    series.setStart($start);
                    series.setLast($last);
                    $valrange2 = series;
                }
            | exprseq[start] COMMA exprseq[step] DOTDOT {
                    // Another case that needs an implied last value.
                    auto series = hadron::library::SeriesNode::make(threadContext, $DOTDOT);
                    series.setStart($start);
                    series.setStep($step);
                    $valrange2 = series;
                }
            | exprseq[start] COMMA exprseq[step] DOTDOT exprseq[last] {
                    auto series = hadron::library::SeriesNode::make(threadContext, $DOTDOT);
                    series.setStart($start);
                    series.setStep($step);
                    series.setLast($last);
                    $valrange2 = series;
                }
            ;

valrange3   : exprseq[start] DOTDOT {
                    auto seriesIter = hadron::library::SeriesIterNode::make(threadContext, $DOTDOT);
                    seriesIter.setStart($start);
                    $valrange3 = seriesIter;
                }
            | DOTDOT exprseq[last] {
                    auto seriesIter = hadron::library::SeriesIterNode::make(threadContext, $DOTDOT);
                    seriesIter.setLast($last);
                    $valrange3 = seriesIter;
                }
            | exprseq[start] DOTDOT exprseq[last] {
                    auto seriesIter = hadron::library::SeriesIterNode::make(threadContext, $DOTDOT);
                    seriesIter.setStart($start);
                    seriesIter.setLast($last);
                    $valrange3 = seriesIter;
                }
            | exprseq[start] COMMA exprseq[step] DOTDOT {
                    auto seriesIter = hadron::library::SeriesIterNode::make(threadContext, $DOTDOT);
                    seriesIter.setStart($start);
                    seriesIter.setStep($step);
                    $valrange3 = seriesIter;
                }
            | exprseq[start] COMMA exprseq[step] DOTDOT exprseq[last] {
                    auto seriesIter = hadron::library::SeriesIterNode::make(threadContext, $DOTDOT);
                    seriesIter.setStart($start);
                    seriesIter.setStep($step);
                    seriesIter.setLast($last);
                    $valrange3 = seriesIter;
                }
            ;

expr[target]    : expr1 { $target = $expr1; }
                | valrangeassign
                | CLASSNAME { $target = hadron::library::NameNode::make(threadContext, $CLASSNAME).toBase(); }
                | expr[leftHand] binop2 adverb expr[rightHand] %prec BINOP {
                        auto binop = hadron::library::BinopCallNode::make(threadContext, $binop2);
                        binop.setLeftHand($leftHand);
                        binop.setRightHand($rightHand);
                        binop.setAdverb($adverb);
                        $target = binop.toBase();
                    }
                | IDENTIFIER ASSIGN expr[build] {
                        auto assign = hadron::library::AssignNode::make(threadContext, $ASSIGN);
                        assign.setName(hadron::library::NameNode::make(threadContext, $IDENTIFIER));
                        assign.setValue($build);
                        $target = assign.toBase();
                    }
                | TILDE IDENTIFIER ASSIGN expr[build] {
                        auto envirPut = hadron::library::EnvironmentPutNode::make(threadContext, $IDENTIFIER);
                        envirPut.setValue($build);
                        $target = envirPut.toBase();
                    }
                | expr[build] DOT IDENTIFIER ASSIGN expr[value] {
                        auto setter = hadron::library::SetterNode::make(threadContext, $IDENTIFIER);
                        setter.setTarget($build);
                        setter.setValue($value);
                        $target = setter.toBase();
                    }
                | IDENTIFIER OPENPAREN arglist1 optkeyarglist CLOSEPAREN ASSIGN expr[value] {
                        if ($optkeyarglist) {
                            // TODO: LSC error condition "Setter method called with keyword arguments"
                        }
                        auto setter = hadron::library::SetterNode::make(threadContext, $IDENTIFIER);
                        setter.setTarget($arglist1.toBase());
                        setter.setValue($value);
                        $target = setter.toBase();
                    }
                | HASH mavars ASSIGN expr[value] {
                        auto multiAssign = hadron::library::MultiAssignNode::make(threadContext, $ASSIGN);
                        multiAssign.setTargets($mavars);
                        multiAssign.setValue($value);
                        $target = multiAssign.toBase();
                    }
                | expr1 OPENSQUARE arglist1 CLOSESQUARE ASSIGN expr[value] {
                        auto write = hadron::library::ArrayWriteNode::make(threadContext, $ASSIGN);
                        write.setTargetArray($expr1);
                        write.setIndexArgument($arglist1);
                        write.setValue($value);
                        $target = write.toBase();
                    }
                ;

block   : OPENCURLY argdecls funcvardecls methbody CLOSECURLY {
                auto block = hadron::library::BlockNode::make(threadContext, $OPENCURLY);
                block.setArguments($argdecls);
                block.setVariables($funcvardecls);
                block.setBody($methbody);
                $block = block;
            }
        | BEGINCLOSEDFUNC argdecls funcvardecls funcbody CLOSECURLY {
                auto block = hadron::library::BlockNode::make(threadContext, $BEGINCLOSEDFUNC);
                block.setArguments($argdecls);
                block.setVariables($funcvardecls);
                block.setBody($funcbody);
                $block = block;
            }
        ;

funcvardecls[target]    : %empty { $target = hadron::library::VarListNode(); }
                        | funcvardecls[build] funcvardecl {
                                $target = append($build, $funcvardecl);
                            }
                        ;

funcvardecls1[target]   : funcvardecl { $target = $funcvardecl; }
                        | funcvardecls1[build] funcvardecl { $target = append($build, $funcvardecl); }
                        ;

funcvardecl : VAR vardeflist SEMICOLON {
                    auto varList = hadron::library::VarListNode::make(threadContext, $VAR);
                    varList.setDefinitions($vardeflist);
                    $funcvardecl = varList;
                }
            ;

argdecls    : %empty { $argdecls = hadron::library::ArgListNode(); }
            | ARG vardeflist SEMICOLON {
                    auto argList = hadron::library::ArgListNode::make(threadContext, $ARG);
                    auto varList = hadron::library::VarListNode::make(threadContext, $ARG);
                    varList.setDefinitions($vardeflist);
                    argList.setVarList(varList);
                    $argdecls = argList;
                }
            | ARG vardeflist0 ELLIPSES IDENTIFIER SEMICOLON {
                    auto argList = hadron::library::ArgListNode::make(threadContext, $ARG);
                    auto varList = hadron::library::VarListNode::make(threadContext, $ARG);
                    varList.setDefinitions($vardeflist0);
                    argList.setVarList(varList);
                    argList.setVarArgsNameToken($IDENTIFIER);
                    $argdecls = argList;
                }
            | PIPE[open] slotdeflist PIPE {
                    auto argList = hadron::library::ArgListNode::make(threadContext, $open);
                    auto varList = hadron::library::VarListNode::make(threadContext, $open);
                    varList.setDefinitions($slotdeflist);
                    argList.setVarList(varList);
                    $argdecls = argList;
                }
            | PIPE[open] slotdeflist0 ELLIPSES IDENTIFIER PIPE {
                    auto argList = hadron::library::ArgListNode::make(threadContext, $open);
                    auto varList = hadron::library::VarListNode::make(threadContext, $open);
                    varList.setDefinitions($slotdeflist0);
                    argList.setVarList(varList);
                    argList.setVarArgsNameToken($IDENTIFIER);
                    $argdecls = argList;
                }
            ;

constdeflist[target]    : constdef { $target = $constdef; }
                        | constdeflist[build] optcomma constdef {
                                $target = append($build, $constdef);
                            }
                        ;

constdef    : rspec IDENTIFIER ASSIGN literal {
                    auto constDef = hadron::library::VarDefNode::make(threadContext, $IDENTIFIER);
                    constDef.setHasReadAccessor($rspec);
                    constDef.setInitialValue($literal);
                    $constdef = constDef;
                }
            ;

slotdeflist0    : %empty { $slotdeflist0 = hadron::library::VarDefNode(); }
                | slotdeflist { $slotdeflist0 = $slotdeflist; }
                ;

slotdeflist[target] : slotdef { $target = $slotdef; }
                    | slotdeflist[build] optcomma slotdef { $target = append($build, $slotdef); }
                    ;

slotdef : IDENTIFIER { $slotdef = hadron::library::VarDefNode::make(threadContext, $IDENTIFIER); }
        | IDENTIFIER optequal literal {
                auto varDef = hadron::library::VarDefNode::make(threadContext, $IDENTIFIER);
                varDef.setInitialValue($literal);
                $slotdef = varDef;
            }
        | IDENTIFIER optequal OPENPAREN exprseq CLOSEPAREN {
                auto varDef = hadron::library::VarDefNode::make(threadContext, $IDENTIFIER);
                varDef.setInitialValue($exprseq.toBase());
                $slotdef = varDef;
            }
        ;

vardeflist0 : %empty { $vardeflist0 = hadron::library::VarDefNode(); }
            | vardeflist { $vardeflist0 = $vardeflist; }
            ;

vardeflist[target]  : vardef { $target = $vardef; }
                    | vardeflist[build] COMMA vardef { $target = append($build, $vardef); }
                    ;

vardef  : IDENTIFIER { $vardef = hadron::library::VarDefNode::make(threadContext, $IDENTIFIER); }
        | IDENTIFIER ASSIGN expr {
                auto varDef = hadron::library::VarDefNode::make(threadContext, $IDENTIFIER);
                varDef.setInitialValue($expr);
                $vardef = varDef;
            }
        | IDENTIFIER OPENPAREN exprseq CLOSEPAREN {
                auto varDef = hadron::library::VarDefNode::make(threadContext, $IDENTIFIER);
                varDef.setInitialValue($exprseq.toBase());
                $vardef = varDef;
            }
        ;

dictslotdef  : exprseq[build] COLON exprseq[next] { $dictslotdef = append($build, $next); }
             | KEYWORD exprseq {
                    auto symbol = hadron::library::SymbolNode::make(threadContext, $KEYWORD);
                    auto exprSeq = hadron::library::ExprSeqNode::make(threadContext, $KEYWORD);
                    exprSeq.setExpr(symbol.toBase());
                    $dictslotdef = append(exprSeq, $exprseq);
                }
             ;

dictslotlist1[target]   : dictslotdef { $target = $dictslotdef; }
                        | dictslotlist1[build] COMMA dictslotdef { $target = append($build, $dictslotdef); }
                        ;

dictslotlist    : %empty { $dictslotlist = hadron::library::ExprSeqNode(); }
                | dictslotlist1 optcomma { $dictslotlist = $dictslotlist1; }
                ;

dictlit2: OPENPAREN litdictslotlist CLOSEPAREN {
                auto litDict = hadron::library::EventNode::make(threadContext, $OPENPAREN);
                litDict.setElements($litdictslotlist.toBase());
                $dictlit2 = litDict;
            }
        ;

litdictslotdef  : listliteral[key] ':' listliteral[value] {
                        auto keyValue = hadron::library::KeyValueNode::make(threadContext, $key.token());
                        keyValue.setKey($key);
                        keyValue.setValue($value);
                        $litdictslotdef = keyValue;
                    }
                | KEYWORD listliteral {
                        auto keyValue = hadron::library::KeyValueNode::make(threadContext, $KEYWORD);
                        auto key = hadron::library::SymbolNode::make(threadContext, $KEYWORD);
                        keyValue.setKey(key.toBase());
                        keyValue.setValue($listliteral);
                        $litdictslotdef = keyValue;
                    }
                ;

litdictslotlist1[target]    : litdictslotdef { $target = $litdictslotdef; }
                            | litdictslotlist1[build] ',' litdictslotdef { $target = append($build, $litdictslotdef); }
                            ;

litdictslotlist : %empty { $litdictslotlist = hadron::library::KeyValueNode(); }
                | litdictslotlist1 optcomma { $litdictslotlist = $litdictslotlist1; }
                ;


listlit : HASH listlit2 { $listlit = $listlit2; }
        ;

// Same as listlit but without the hashes, for inner literal lists
listlit2 : OPENSQUARE literallistc CLOSESQUARE {
                auto litList = hadron::library::CollectionNode::make(threadContext, $OPENSQUARE);
                litList.setElements($literallistc);
                $listlit2 = litList;
            }
        | CLASSNAME OPENSQUARE literallistc CLOSESQUARE {
                auto litList = hadron::library::CollectionNode::make(threadContext, $OPENSQUARE);
                auto nameNode = hadron::library::NameNode::make(threadContext, $CLASSNAME);
                litList.setClassName(nameNode);
                litList.setElements($literallistc.toBase());
                $listlit2 = litList;
            }
        ;

literallistc    : %empty { $literallistc = hadron::library::Node(); }
                | literallist1 optcomma { $literallistc = $literallist1; }
                ;

literallist1[target]    : listliteral { $target = $listliteral; }
                        | literallist1[build] COMMA listliteral { $target = append($build, $listliteral); }
                        ;

rwslotdeflist[target]   : rwslotdef { $target = $rwslotdef; }
                        | rwslotdeflist[build] COMMA rwslotdef { $target = append($build, $rwslotdef); }
                        ;

rwslotdef   : rwspec IDENTIFIER {
                    auto varDef = hadron::library::VarDefNode::make(threadContext, $IDENTIFIER);
                    varDef.setHasReadAccessor($rwspec.first);
                    varDef.setHasWriteAccessor($rwspec.second);
                    $rwslotdef = varDef;
                }
            | rwspec IDENTIFIER ASSIGN literal {
                    auto varDef = hadron::library::VarDefNode::make(threadContext, $IDENTIFIER);
                    varDef.setHasReadAccessor($rwspec.first);
                    varDef.setHasWriteAccessor($rwspec.second);
                    varDef.setInitialValue($literal.toBase());
                    $rwslotdef = varDef;
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

arglist1[target]    : exprseq { $target = $exprseq; }
                    | arglist1[build] COMMA exprseq { $target = append($build, $exprseq); }
                    ;

arglistv1   : ASTERISK exprseq { $arglistv1 = $exprseq; }
            | arglist1 COMMA ASTERISK exprseq { $arglistv1 = append($arglist1, $exprseq); }
            ;

keyarglist1[target] : keyarg { $target = $keyarg; }
                    | keyarglist1[build] COMMA keyarg { $target = append($build, $keyarg); }
                    ;

keyarg  : KEYWORD exprseq {
                auto keyArg = hadron::library::KeyValueNode::make(threadContext, $KEYWORD);
                auto key = hadron::library::SymbolNode::make(threadContext, $KEYWORD);
                keyArg.setKey(key.toBase());
                keyArg.setValue($exprseq.toBase());
                $keyarg = keyArg;
            }
        ;

optkeyarglist   : optcomma { $optkeyarglist = hadron::library::KeyValueNode(); }
                | COMMA keyarglist1 optcomma { $optkeyarglist = $keyarglist1; }
                ;

mavars  : mavarlist {
                auto multiVars = hadron::library::MultiAssignVarsNode::make(threadContext, $mavarlist.token());
                multiVars.setNames($mavarlist);
                $mavars = multiVars;
            }
        | mavarlist ELLIPSES IDENTIFIER {
                auto multiVars = hadron::library::MultiAssignVarsNode::make(threadContext, $mavarlist.token());
                multiVars.setNames($mavarlist);
                auto rest = hadron::library::NameNode::make(threadContext, $IDENTIFIER);
                multiVars.setRest(rest);
                $mavars = multiVars;
            }
        ;

mavarlist[target]   : IDENTIFIER {
                            auto name = hadron::library::NameNode::make(threadContext, $IDENTIFIER);
                            $target = name;
                        }
                    | mavarlist[build] COMMA IDENTIFIER {
                            auto name = hadron::library::NameNode::make(threadContext, $IDENTIFIER);
                            $target = append($build, name);
                        }
                    ;

blockliteral:   block { $blockliteral = $block; }
            ;

listliteral : coreliteral { $listliteral = $coreliteral; }
        | listlit2 { $listliteral = $listlit2.toBase(); }
        | dictlit2 { $listliteral = $dictlit2.toBase(); }
        // NOTE: within lists symbols can be specified without \ or '' notation.
        | IDENTIFIER { $listliteral = hadron::library::SymbolNode::make(threadContext, $IDENTIFIER).toBase(); }
        ;

literal : coreliteral { $literal = $coreliteral; }
        | listlit { $literal = $listlit.toBase(); }
        ;

coreliteral : LITERAL {
                    auto literal = hadron::library::SlotNode::make(threadContext, $LITERAL);
                    literal.setValue($LITERAL.value());
                    $coreliteral = literal.toBase();
                }
            | integer {
                    auto literal = hadron::library::SlotNode::make(threadContext, $integer.first);
                    literal.setValue(hadron::Slot::makeInt32($integer.second));
                    $coreliteral = literal.toBase();
                }
            | float {
                    auto literal = hadron::library::SlotNode::make(threadContext, $float.first);
                    literal.setValue(hadron::Slot::makeFloat($float.second));
                    $coreliteral = literal.toBase();
                }
            | stringlist { $coreliteral = $stringlist.toBase(); }
            | SYMBOL { $coreliteral = hadron::library::SymbolNode::make(threadContext, $SYMBOL).toBase(); }
            ;

integer : INTEGER { $integer = std::make_pair($INTEGER, $INTEGER.value().getInt32()); }
        | MINUS INTEGER %prec MINUS { $integer = std::make_pair($INTEGER, -$INTEGER.value().getInt32()); }
        ;

floatr  : FLOAT { $floatr = std::make_pair($FLOAT, $FLOAT.value().getFloat()); }
        | MINUS FLOAT %prec MINUS { $floatr = std::make_pair($FLOAT, -$FLOAT.value().getFloat()); }
        ;

float   : floatr { $float = $floatr; }
        | floatr PI { $float = std::make_pair($floatr.first, $floatr.second * M_PI); }
        | integer PI { $float = std::make_pair($integer.first, static_cast<double>($integer.second) * M_PI); }
        | PI { $float = std::make_pair($PI, M_PI); }
        | MINUS PI { $float = std::make_pair($PI, -M_PI); }
        ;

binop   : BINOP { $binop = $BINOP; }
        | PLUS { $binop = $PLUS; }
        | MINUS { $binop = $MINUS; }
        | ASTERISK { $binop = $ASTERISK; }
        | LESSTHAN { $binop = $LESSTHAN; }
        | GREATERTHAN { $binop = $GREATERTHAN; }
        | PIPE { $binop = $PIPE; }
        | READWRITEVAR { $READWRITEVAR; }
        ;

binop2  : binop { $binop2 = $binop; }
        | KEYWORD { $binop2 = $KEYWORD; }
        ;

string  : STRING { $string = hadron::library::StringNode::make(threadContext, $STRING); }
        ;

stringlist[target]  : string { $target = $string; }
                    | stringlist[build] string { $target = append($build, $string); }
                    ;
%%

yy::parser::symbol_type yylex(hadron::Parser* hadronParser, hadron::ThreadContext* threadContext) {
    if (hadronParser->sendInterpret()) {
        hadronParser->setInterpret(false);
        return yy::parser::make_INTERPRET(hadron::Token::Location{0, 0});
    }

    auto index = hadronParser->tokenIndex();
    hadron::Token token = hadronParser->token(index);
    hadronParser->next();

    auto scToken = hadron::library::Token::alloc(threadContext);
    scToken.setValue(token.value);
    scToken.setLineNumber(token.location.lineNumber + 1);
    scToken.setCharacterNumber(token.location.characterNumber + 1);
    scToken.setOffset(token.range.data() - hadronParser->lexer()->code().data());
    scToken.setLength(token.range.size());
    scToken.setSnippet(hadron::library::Symbol::fromView(threadContext, token.range));
    scToken.setHasEscapeCharacters(token.escapeString);

    switch (token.name) {
    case hadron::Token::Name::kEmpty:
        return yy::parser::make_END(token.location);

    case hadron::Token::Name::kInterpret:
        return yy::parser::make_INTERPRET(token.location);

    case hadron::Token::Name::kLiteral:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "literal"));
        // We special-case integers and floats to support unary negation.
        if (token.value.isInt32()) {
            return yy::parser::make_INTEGER(scToken, token.location);
        } else if (token.value.isFloat()) {
            return  yy::parser::make_FLOAT(scToken, token.location);
        }
        return yy::parser::make_LITERAL(scToken, token.location);

    case hadron::Token::Name::kPi:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "pi"));
        return yy::parser::make_PI(scToken, token.location);

    case hadron::Token::Name::kString:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "string"));
        return yy::parser::make_STRING(scToken, token.location);

    case hadron::Token::Name::kSymbol:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "symbol"));
        return yy::parser::make_SYMBOL(scToken, token.location);

    case hadron::Token::Name::kPrimitive:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "primitive"));
        return yy::parser::make_PRIMITIVE(scToken, token.location);

    case hadron::Token::Name::kPlus:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "plus"));
        return yy::parser::make_PLUS(scToken, token.location);

    case hadron::Token::Name::kMinus:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "minus"));
        return yy::parser::make_MINUS(scToken, token.location);

    case hadron::Token::Name::kAsterisk:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "asterisk"));
        return yy::parser::make_ASTERISK(scToken, token.location);

    case hadron::Token::Name::kAssign:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "assign"));
        return yy::parser::make_ASSIGN(scToken, token.location);

    case hadron::Token::Name::kLessThan:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "lessThan"));
        return yy::parser::make_LESSTHAN(scToken, token.location);

    case hadron::Token::Name::kGreaterThan:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "greaterThan"));
        return yy::parser::make_GREATERTHAN(scToken, token.location);

    case hadron::Token::Name::kPipe:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "pipe"));
        return yy::parser::make_PIPE(scToken, token.location);

    case hadron::Token::Name::kReadWriteVar:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "readWriteVar"));
        return yy::parser::make_READWRITEVAR(scToken, token.location);

    case hadron::Token::Name::kLeftArrow:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "leftArrow"));
        return yy::parser::make_LEFTARROW(scToken, token.location);

    case hadron::Token::Name::kBinop:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "binop"));
        return yy::parser::make_BINOP(scToken, token.location);

    case hadron::Token::Name::kKeyword:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "keyword"));
        return yy::parser::make_KEYWORD(scToken, token.location);

    case hadron::Token::Name::kOpenParen:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "openParen"));
        return yy::parser::make_OPENPAREN(scToken, token.location);

    case hadron::Token::Name::kCloseParen:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "closeParen"));
        return yy::parser::make_CLOSEPAREN(scToken, token.location);

    case hadron::Token::Name::kOpenCurly:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "openCurly"));
        return yy::parser::make_OPENCURLY(scToken, token.location);

    case hadron::Token::Name::kCloseCurly:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "closeCurly"));
        return yy::parser::make_CLOSECURLY(scToken, token.location);

    case hadron::Token::Name::kOpenSquare:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "openSquare"));
        return yy::parser::make_OPENSQUARE(scToken, token.location);

    case hadron::Token::Name::kCloseSquare:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "closeSquare"));
        return yy::parser::make_CLOSESQUARE(scToken, token.location);

    case hadron::Token::Name::kComma:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "comma"));
        return yy::parser::make_COMMA(scToken, token.location);

    case hadron::Token::Name::kSemicolon:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "semicolon"));
        return yy::parser::make_SEMICOLON(scToken, token.location);

    case hadron::Token::Name::kColon:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "colon"));
        return yy::parser::make_COLON(scToken, token.location);

    case hadron::Token::Name::kCaret:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "caret"));
        return yy::parser::make_CARET(scToken, token.location);

    case hadron::Token::Name::kTilde:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "tilde"));
        return yy::parser::make_TILDE(scToken, token.location);

    case hadron::Token::Name::kHash:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "hash"));
        return yy::parser::make_HASH(scToken, token.location);

    case hadron::Token::Name::kGrave:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "grave"));
        return yy::parser::make_GRAVE(scToken, token.location);

    case hadron::Token::Name::kVar:
        scToken.setName(threadContext->symbolTable->varSymbol());
        return yy::parser::make_VAR(scToken, token.location);

    case hadron::Token::Name::kArg:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "arg"));
        return yy::parser::make_ARG(scToken, token.location);

    case hadron::Token::Name::kConst:
        scToken.setName(threadContext->symbolTable->constSymbol());
        return yy::parser::make_CONST(scToken, token.location);

    case hadron::Token::Name::kClassVar:
        scToken.setName(threadContext->symbolTable->classvarSymbol());
        return yy::parser::make_CLASSVAR(scToken, token.location);

    case hadron::Token::Name::kIdentifier:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "identifier"));
        return yy::parser::make_IDENTIFIER(scToken, token.location);

    case hadron::Token::Name::kClassName:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "className"));
        return yy::parser::make_CLASSNAME(scToken, token.location);

    case hadron::Token::Name::kDot:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "dot"));
        return yy::parser::make_DOT(scToken, token.location);

    case hadron::Token::Name::kDotDot:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "dotDot"));
        return yy::parser::make_DOTDOT(scToken, token.location);

    case hadron::Token::Name::kEllipses:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "ellipses"));
        return yy::parser::make_ELLIPSES(scToken, token.location);

    case hadron::Token::Name::kCurryArgument:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "curryArgument"));
        return yy::parser::make_CURRYARGUMENT(scToken, token.location);

    case hadron::Token::Name::kBeginClosedFunction:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "beginClosedFunction"));
        return yy::parser::make_BEGINCLOSEDFUNC(scToken, token.location);

    case hadron::Token::Name::kIf:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "if"));
        return yy::parser::make_IF(scToken, token.location);

    case hadron::Token::Name::kWhile:
        scToken.setName(hadron::library::Symbol::fromView(threadContext, "while"));
        return yy::parser::make_WHILE(scToken, token.location);
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

bool Parser::parse(ThreadContext* context) {
    m_sendInterpret = true;
    return innerParse(context);
}

bool Parser::parseClass(ThreadContext* context) {
    m_sendInterpret = false;
    return innerParse(context);
}

void Parser::addRoot(hadron::library::Node root) {
    if (root) {
        if (m_root) {
            m_root.append(root);
        } else {
            m_root = root;
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
    if (m_ownLexer) {
        if (!m_lexer->lex()) {
            return false;
        }
    }

    m_root = hadron::library::Node();
    m_tokenIndex = 0;
    yy::parser parser(this, context);
    auto error = parser.parse();
    if (error != 0) {
        return false;
    }

    // Valid parse of empty input buffer, which we represent with an empty node.
    if (!m_root) {
        m_root = hadron::library::EmptyNode::make(context, hadron::library::Token()).toBase();
    }

    return m_errorReporter->errorCount() == 0;
}

} // namespace hadron