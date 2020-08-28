Legacy SuperCollider grammar extracted from the Bison file:

```
%token    NAME INTEGER SC_FLOAT ACCIDENTAL SYMBOL STRING ASCII PRIMITIVENAME CLASSNAME CURRYARG
%token  VAR ARG CLASSVAR SC_CONST
%token    NILOBJ TRUEOBJ FALSEOBJ
%token    PSEUDOVAR
%token  ELLIPSIS DOTDOT PIE BEGINCLOSEDFUNC
%token  BADTOKEN INTERPRET
%token  BEGINGENERATOR LEFTARROW WHILE
%left    ':'
%right  '='
%left    BINOP KEYBINOP '-' '<' '>' '*' '+' '|' READWRITEVAR
%left    '.'
%right  '`'
%right  UMINUS
%start  root

root: classes | classextensions | INTERPRET cmdlinecode

classes: <e> | classes classdef

classextensions: classextension | classextensions classextension

classdef: classname superclass '{' classvardecls methods '}'
            | classname '[' optname ']' superclass '{' classvardecls methods '}'

classextension: '+' classname '{' methods '}'

optname:        <e> | name

superclass: <e> | ':' classname

classvardecls: <e> | classvardecls classvardecl

classvardecl: CLASSVAR rwslotdeflist ';'
                | VAR rwslotdeflist ';'
                | SC_CONST constdeflist ';'

methods: <e> | methods methoddef

methoddef: name '{' argdecls funcvardecls primitive methbody '}'
            | '*' name '{' argdecls funcvardecls primitive methbody '}'
            | binop '{' argdecls funcvardecls primitive methbody '}'
            | '*' binop '{' argdecls funcvardecls primitive methbody '}'

optsemi: <e> | ';'

optcomma: <e> | ','

optequal: <e> | '='

funcbody: funretval
            | exprseq funretval

cmdlinecode: '(' funcvardecls1 funcbody ')'
             | funcvardecls1 funcbody
             | funcbody

methbody: retval | exprseq retval

primitive: <e> | primname optsemi

retval: <e> | '^' expr optsemi

funretval: <e> | '^' expr optsemi

blocklist1: blocklistitem | blocklist1 blocklistitem

blocklistitem : blockliteral | generator

blocklist: <e> | blocklist1

msgsend: name blocklist1
        | '(' binop2 ')' blocklist1
        | name '(' ')' blocklist1
        | name '(' arglist1 optkeyarglist ')' blocklist
        | '(' binop2 ')' '(' ')' blocklist1
        | '(' binop2 ')' '(' arglist1 optkeyarglist ')' blocklist
        | name '(' arglistv1 optkeyarglist ')'
        | '(' binop2 ')' '(' arglistv1 optkeyarglist ')'
        | classname '[' arrayelems ']'
        | classname blocklist1
        | classname '(' ')' blocklist
        | classname '(' keyarglist1 optcomma ')' blocklist
        | classname '(' arglist1 optkeyarglist ')' blocklist
        | classname '(' arglistv1 optkeyarglist ')'
        | expr '.' '(' ')' blocklist
        | expr '.' '(' keyarglist1 optcomma ')' blocklist
        | expr '.' name '(' keyarglist1 optcomma ')' blocklist
        | expr '.' '(' arglist1 optkeyarglist ')' blocklist
        | expr '.' '(' arglistv1 optkeyarglist ')'
        | expr '.' name '(' ')' blocklist
        | expr '.' name '(' arglist1 optkeyarglist ')' blocklist
        | expr '.' name '(' arglistv1 optkeyarglist ')'
        | expr '.' name blocklist

generator: '{' ':' exprseq ',' qual '}'
        | '{' ';' exprseq  ',' qual '}'

nextqual: <e> | ',' qual

qual: name LEFTARROW exprseq nextqual
        | name name LEFTARROW exprseq nextqual
        | VAR name '=' exprseq nextqual
        | exprseq nextqual
        | ':' ':' exprseq nextqual
        | ':' WHILE exprseq nextqual

expr1: pushliteral
        | blockliteral
        | generator
        | pushname
        | curryarg
        | msgsend
        | '(' exprseq ')'
        | '~' name
        |  '[' arrayelems ']'
        |    '(' valrange2 ')'
        |    '(' ':' valrange3 ')'
        |    '(' dictslotlist ')'
        | pseudovar
        | expr1 '[' arglist1 ']'
        | valrangex1

valrangex1: expr1 '[' arglist1 DOTDOT ']'
        | expr1 '[' DOTDOT exprseq ']'
        | expr1 '[' arglist1 DOTDOT exprseq ']'

valrangeassign: expr1 '[' arglist1 DOTDOT ']' '=' expr
        | expr1 '[' DOTDOT exprseq ']' '=' expr
        | expr1 '[' arglist1 DOTDOT exprseq ']' '=' expr

valrangexd: expr '.' '[' arglist1 DOTDOT ']'
        | expr '.' '[' DOTDOT exprseq ']'
        | expr '.' '[' arglist1 DOTDOT exprseq ']'
        | expr '.' '[' arglist1 DOTDOT ']' '=' expr
        | expr '.' '[' DOTDOT exprseq ']' '=' expr
        | expr '.' '[' arglist1 DOTDOT exprseq ']' '=' expr

valrange2: exprseq DOTDOT
        | DOTDOT exprseq
        | exprseq DOTDOT exprseq
        | exprseq ',' exprseq DOTDOT exprseq
        | exprseq ',' exprseq DOTDOT

valrange3: DOTDOT exprseq
        | exprseq DOTDOT
        | exprseq DOTDOT exprseq
        | exprseq ',' exprseq DOTDOT
        | exprseq ',' exprseq DOTDOT exprseq

expr: expr1
        | valrangexd
        | valrangeassign
        | classname
        | expr '.' '[' arglist1 ']'
        | '`' expr
        | expr binop2 adverb expr %prec BINOP
        | name '=' expr
        | '~' name '=' expr
        | expr '.' name '=' expr
        | name '(' arglist1 optkeyarglist ')' '=' expr
        | '#' mavars '=' expr
        | expr1 '[' arglist1 ']' '=' expr
        | expr '.' '[' arglist1 ']' '=' expr

adverb: <e>
        | '.' name
        | '.' integer
        | '.' '(' exprseq ')'

exprn: expr | exprn ';' expr

exprseq: exprn optsemi

arrayelems: <e> | arrayelems1 optcomma

arrayelems1: exprseq
             | exprseq ':' exprseq
             | keybinop exprseq
             | arrayelems1 ',' exprseq
             | arrayelems1 ',' keybinop exprseq
             | arrayelems1 ',' exprseq ':' exprseq

arglist1: exprseq | arglist1 ',' exprseq

arglistv1: '*' exprseq | arglist1 ',' '*' exprseq

keyarglist1: keyarg | keyarglist1 ',' keyarg

keyarg: keybinop exprseq

optkeyarglist: optcomma | ',' keyarglist1 optcomma

mavars: mavarlist | mavarlist ELLIPSIS name

mavarlist: name | mavarlist ',' name

slotliteral: integer | floatp | ascii | string | symbol | trueobj | falseobj | nilobj | listlit

blockliteral: block

pushname: name

pushliteral: integer | floatp | ascii | string | symbol | trueobj | falseobj | nilobj | listlit

listliteral: integer | floatp | ascii | string | symbol | name | trueobj | falseobj | nilobj | listlit2 | dictlit2

block: '{' argdecls funcvardecls funcbody '}'
        | BEGINCLOSEDFUNC argdecls funcvardecls funcbody '}'

funcvardecls: <e> | funcvardecls funcvardecl

funcvardecls1: funcvardecl | funcvardecls1 funcvardecl

funcvardecl: VAR vardeflist ';'

argdecls: <e> | ARG vardeflist ';'
              | ARG vardeflist0 ELLIPSIS name ';'
              | '|' slotdeflist '|'
              | '|' slotdeflist0 ELLIPSIS name '|'

constdeflist: constdef | constdeflist optcomma constdef

constdef: rspec name '=' slotliteral

slotdeflist0: <e> | slotdeflist

slotdeflist: slotdef
             | slotdeflist optcomma slotdef

slotdef: name
         | name optequal slotliteral
         | name optequal '(' exprseq ')'

vardeflist0: <e> | vardeflist
vardeflist: vardef | vardeflist ',' vardef
vardef: name | name '=' expr | name '(' exprseq ')'

dictslotdef: exprseq ':' exprseq | keybinop exprseq

dictslotlist1: dictslotdef | dictslotlist1 ',' dictslotdef

dictslotlist: <e> | dictslotlist1 optcomma

rwslotdeflist: rwslotdef | rwslotdeflist ',' rwslotdef

rwslotdef: rwspec name | rwspec name '=' slotliteral

dictlit2: '(' litdictslotlist ')'

litdictslotdef: listliteral ':' listliteral | keybinop listliteral

litdictslotlist1: litdictslotdef | litdictslotlist1 ',' litdictslotdef

litdictslotlist: <e> | litdictslotlist1 optcomma

listlit: '#' '[' literallistc ']' | '#' classname  '[' literallistc ']'

listlit2: '[' literallistc ']' | classname  '[' literallistc ']'

literallistc: <e> | literallist1 optcomma

literallist1: listliteral | literallist1 ',' listliteral

rwspec: <e> | '<' | READWRITEVAR | '>'

rspec:  <e> | '<'

integer: INTEGER | '-'INTEGER %prec UMINUS

floatr: SC_FLOAT | '-' SC_FLOAT %prec UMINUS

accidental: ACCIDENTAL | '-' ACCIDENTAL %prec UMINUS

pie: PIE

floatp: floatr | accidental | floatr pie | integer pie | pie | '-' pie

name: NAME | WHILE

classname: CLASSNAME

primname: PRIMITIVENAME

trueobj: TRUEOBJ

falseobj: FALSEOBJ

nilobj: NILOBJ

ascii: ASCII

symbol: SYMBOL

string: STRING

pseudovar: PSEUDOVAR

binop: BINOP | READWRITEVAR | '<' | '>' | '-' | '*' | '+' | '|'

keybinop: KEYBINOP

binop2: binop | keybinop

curryarg: CURRYARG
```
