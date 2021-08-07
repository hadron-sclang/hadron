%require "3.2"
%language "c++"
%define api.value.type variant
%token <hadron::Slot> INTEGER

%type<std::unique_ptr<hadron::parse::Node>> root funcbody expr exprn
%type<std::unique_ptr<hadron::parse::BlockNode>> cmdlinecode
%type<std::unique_ptr<hadron::parse::ClassNode>> classes classdef
%type<std::unique_ptr<hadron::parse::ReturnNode>> funretval
%type<std::unique_ptr<hadron::parse::ExprSeqNode>> exprseq
%type<std::unique_ptr<hadron::parse::LiteralNode>> integer

%{
#include "hadron/ParserEngine.hpp"

#include "hadron/Parser.hpp"
#include "hadron/Slot.hpp"
#include "hadron/Token.hpp"

#include <memory>
%}

%%
root    : classes
            { m_root = $classes; }
        | cmdlinecode
            { m_root = $cmdlinecode; }
        ;

classes[target] : { m_root = nullptr; }
//                | classes[build] classdef
//                    {
//                        $build->append($classdef);
//                        $target = std::move($build);
//                    }
                ;

cmdlinecode : funcbody
                {
                    auto block = std::make_unique<hadron::parse::BlockNode>(m_index);
                    block->body = $funcbody;
                    $cmdlinecode = std::move(block);
                }
            ;

funcbody    : funretval
                { $funcbody = $funretval; }
            | exprseq funretval
                {
                    auto exprSeq = std::make_unique<hadron::parse::ExprSeqNode>(m_index);
                    exprSeq->expr = $exprseq;
                    exprSeq->expr->append($funretval);
                    $funcbody = std::move(exprSeq);
                }

funretval   :
                { $funretval = std::make_unique<hadron::parse::ReturnNode>(m_index); }
            | '^' expr optsemi
                {
                    auto ret = std::make_unique<hadron::parse::ReturnNode>(m_index);
                    ret->valueExpr = $expr;
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

expr[target]    : integer
                    { $target = std::make_unique<hadron::parse::LiteralNode>(m_index, $integer); }
                ;

integer : INTEGER
            { $integer = hadron::Slot(hadron::Type::kInteger, hadron::Slot::Value(zzval)); }
%%