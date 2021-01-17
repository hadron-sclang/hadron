%%{
    machine grammarNode;

    main := |*
        # Any lower-case pattern without a colon at the end is treated as a reference to a new rule.
        lower alnum* {
            std::string ruleName(ts, te - ts);
            currentPattern->isRecursive |= ruleName == currentRule->name;
            currentPattern->termNames.emplace_back(ruleName);
        };

        # A colon at the end of the name means this token defines the name of a new rule.
        lower alnum* ':' {
            std::string name(ts, te - ts - 1);
            auto mapEntry = m_ruleMap.emplace(name, std::make_unique<GrammarRule>(name));
            if (!mapEntry.second) {
                spdlog::error("Error in grammar, the rule named '{}' appears more than once", name);
                return false;
            }
            currentRule = mapEntry.first->second.get();
            currentPattern = currentRule->patterns.emplace_back(std::make_unique<GrammarPattern>()).get();
        };

        # Pipes delineate individual patterns for the rules.
        '|' {
            currentPattern = currentRule->patterns.emplace_back(std::make_unique<GrammarPattern>()).get();
            ++patternCount;
        };

        # Anything enclosed in single quotes indicates a token requiring an exact match.
        '\'' (any - '\'')+ '\'' {
            std::string token(ts, te - ts);
            // Insert a simple rule representing this if token is not already present in map.
            if (m_ruleMap.find(token) == m_ruleMap.end()) {
                m_ruleMap.emplace(token, std::make_unique<GrammarRule>(token, std::string(ts + 1, te - ts - 2)));
            }
            currentPattern->termNames.emplace_back(token);
        };

        # The <e> means the rule will match the empty string, so we mark it as optional.
        '<e>' {
            if (currentPattern->termNames.size()) {
                spdlog::error("empty term '<e>' found in non-empty pattern.");
                return false;
            }
            ++patternCount;
        };

        'ACCIDENTAL' {
            m_ruleMap.emplace("ACCIDENTAL", std::make_unique<GrammarRule>("ACCIDENTAL", "b"));
            currentPattern->termNames.emplace_back("ACCIDENTAL");
        };

        'ARG' {
            m_ruleMap.emplace("ARG", std::make_unique<GrammarRule>("ARG", "arg"));
            currentPattern->termNames.emplace_back("ARG");
        };

        'ASCII' {
            m_ruleMap.emplace("ASCII", std::make_unique<GrammarRule>("ASCII", "$c"));
            currentPattern->termNames.emplace_back("ASCII");
        };

        'BEGINCLOSEDFUNC' {
            m_ruleMap.emplace("BEGINCLOSEDFUNC", std::make_unique<GrammarRule>("BEGINCLOSEDFUNC", "#{"));
            currentPattern->termNames.emplace_back("BEGINCLOSEDFUNC");
        };

        'BINOP' {
            m_ruleMap.emplace("BINOP", std::make_unique<GrammarRule>("BINOP", "++"));
            currentPattern->termNames.emplace_back("BINOP");
        };

        'CLASSNAME' {
            m_ruleMap.emplace("CLASSNAME", std::make_unique<GrammarRule>("CLASSNAME", "Cls"));
            currentPattern->termNames.emplace_back("CLASSNAME");
        };

        'CLASSVAR' {
            m_ruleMap.emplace("CLASSVAR", std::make_unique<GrammarRule>("CLASSVAR", "classvar"));
            currentPattern->termNames.emplace_back("CLASSVAR");
        };

        'CURRYARG' {
            m_ruleMap.emplace("CURRYARG", std::make_unique<GrammarRule>("CURRYARG", "_"));
            currentPattern->termNames.emplace_back("CURRYARG");
        };

        'DOTDOT' {
            m_ruleMap.emplace("DOTDOT", std::make_unique<GrammarRule>("DOTDOT", ".."));
            currentPattern->termNames.emplace_back("DOTDOT");
        };

        'ELLIPSIS' {
            m_ruleMap.emplace("ELLIPSIS", std::make_unique<GrammarRule>("ELLIPSIS", "..."));
            currentPattern->termNames.emplace_back("ELLIPSIS");
        };

        'FALSEOBJ' {
            m_ruleMap.emplace("FALSEOBJ", std::make_unique<GrammarRule>("FALSEOBJ", "false"));
            currentPattern->termNames.emplace_back("FALSEOBJ");
        };

        'INTEGER' {
            m_ruleMap.emplace("INTEGER", std::make_unique<GrammarRule>("INTEGER", "1"));
            currentPattern->termNames.emplace_back("INTEGER");
        };

        'KEYBINOP' {
            m_ruleMap.emplace("KEYBINOP", std::make_unique<GrammarRule>("KEYBINOP", "key:"));
            currentPattern->termNames.emplace_back("KEYBINOP");
        };

        'LEFTARROW' {
            m_ruleMap.emplace("LEFTARROW", std::make_unique<GrammarRule>("LEFTARROW", "<-"));
            currentPattern->termNames.emplace_back("LEFTARROW");
        };

        'NAME' {
            m_ruleMap.emplace("NAME", std::make_unique<GrammarRule>("NAME", "x"));
            currentPattern->termNames.emplace_back("NAME");
        };

        'NILOBJ' {
            m_ruleMap.emplace("NILOBJ", std::make_unique<GrammarRule>("NILOBJ", "nil"));
            currentPattern->termNames.emplace_back("NILOBJ");
        };

        'PIE' {
            m_ruleMap.emplace("PIE", std::make_unique<GrammarRule>("PIE", "pi"));
            currentPattern->termNames.emplace_back("PIE");
        };

        'PRIMITIVENAME' {
            m_ruleMap.emplace("PRIMITIVENAME", std::make_unique<GrammarRule>("PRIMITIVENAME", "_Prim"));
            currentPattern->termNames.emplace_back("PRIMITIVENAME");
        };

        'READWRITEVAR' {
            m_ruleMap.emplace("READWRITEVAR", std::make_unique<GrammarRule>("READWRITEVAR", "<>"));
            currentPattern->termNames.emplace_back("READWRITEVAR");
        };

        'SC_CONST' {
            m_ruleMap.emplace("SC_CONST", std::make_unique<GrammarRule>("SC_CONST", "const"));
            currentPattern->termNames.emplace_back("SC_CONST");
        };

        'SC_FLOAT' {
            m_ruleMap.emplace("SC_FLOAT", std::make_unique<GrammarRule>("SC_FLOAT", "1.0"));
            currentPattern->termNames.emplace_back("SC_FLOAT");
        };

        'STRING' {
            m_ruleMap.emplace("STRING", std::make_unique<GrammarRule>("STRING", "\"str\""));
            currentPattern->termNames.emplace_back("STRING");
        };

        'SYMBOL' {
            m_ruleMap.emplace("SYMBOL", std::make_unique<GrammarRule>("SYMBOL", "\\sym"));
            currentPattern->termNames.emplace_back("SYMBOL");
        };

        'TRUEOBJ' {
            m_ruleMap.emplace("TRUEOBJ", std::make_unique<GrammarRule>("TRUEOBJ", "true"));
            currentPattern->termNames.emplace_back("TRUEOBJ");
        };

        'VAR' {
            m_ruleMap.emplace("VAR", std::make_unique<GrammarRule>("VAR", "var"));
            currentPattern->termNames.emplace_back("VAR");
        };

        'WHILE' {
            m_ruleMap.emplace("WHILE", std::make_unique<GrammarRule>("WHILE", "while"));
            currentPattern->termNames.emplace_back("WHILE");
        };

        (upper | '_')+ {
            std::string token(ts, te - ts);
            spdlog::error("Unrecognized token {}", token);
            return false;
        };

        space { /* ignore whitespace */ };
        any {
            std::string token(ts, te - ts);
            spdlog::error("Grammar parse encountered unknown character '{}'.", token);
            return false;
        };
    *|;
}%%

// Generated file from Ragel input file test/GrammarIteratorParser.rl. Please make edits to that file and regenerate.

#include "GrammarIterator.hpp"

#include "spdlog/spdlog.h"

#include <cstring>

namespace {

// TODO: PSEUDOVAR token removed from original grammar, determine what it means.
static const char* kGrammar = R"grammar(
root: classes | classextensions | cmdlinecode
classes: <e> | classes classdef
classextensions: classextension | classextensions classextension
classdef: classname superclass '{' classvardecls methods '}'
        | classname '[' optname ']' superclass '{' classvardecls methods '}'
classextension: '+' classname '{' methods '}'
optname: <e> | name
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
blocklistitem: blockliteral | generator
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
     | '[' arrayelems ']'
     | '(' valrange2 ')'
     | '(' ':' valrange3 ')'
     | '(' dictslotlist ')'
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
    | expr binop2 adverb expr
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
argdecls: <e>
        | ARG vardeflist ';'
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
integer: INTEGER | '-' INTEGER
floatr: SC_FLOAT | '-' SC_FLOAT
accidental: ACCIDENTAL | '-' ACCIDENTAL
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
binop: BINOP | READWRITEVAR | '<' | '>' | '-' | '*' | '+' | '|'
keybinop: KEYBINOP
binop2: binop | keybinop
curryarg: CURRYARG
)grammar";

#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-const-variable"
%% write data;
#   pragma GCC diagnostic pop

} // namespace

namespace hadron {

// static
bool GrammarIterator::buildGrammarTree() {
    size_t len = std::strlen(kGrammar);
    GrammarRule* currentRule = nullptr;
    GrammarPattern* currentPattern = nullptr;
    size_t patternCount = 0;

    // Ragel-required state machine variables.
    const char* p = kGrammar;
    const char* pe = p + len;
    const char* eof = pe;
    int cs;
    int act;
    const char* ts;
    const char* te;

    %% write init;

    %% write exec;

    // We require a root node to exist and be named "root".
    if (m_ruleMap.find("root") == m_ruleMap.end()) {
        spdlog::error("Grammar root rule not found.");
        return false;
    }

    // Now that all rules are parsed we use the rule map to resolve all rule names in patterns to rule pointers.
    for (auto& rulePair : m_ruleMap) {
        GrammarRule* rule = rulePair.second.get();
        for (auto& pattern : rule->patterns) {
            for (const auto& name : pattern->termNames) {
                auto iter = m_ruleMap.find(name);
                if (iter == m_ruleMap.end()) {
                    spdlog::error("Rule {} has pattern with undefined rule name {}", rule->name, name);
                    return false;
                }
                pattern->terms.emplace_back(iter->second.get());
            }
        }
    }

    spdlog::info("Constructed sclang grammar tree with {} rules and {} patterns.", m_ruleMap.size(), patternCount);
    return true;
}

} // namespace hadron
