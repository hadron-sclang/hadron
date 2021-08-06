%require "3.2"
%language "c++"
%token <hadron::Token::Name> INTEGER


%{
#include "hadron/ParserEngine.hpp"
#include "hadron/Token.hpp"
%}

%%
integer: INTEGER
%%