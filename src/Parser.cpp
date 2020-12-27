#include "Parser.hpp"

#include "ErrorReporter.hpp"
#include "Lexer.hpp"

#include <fmt/core.h>

#include <assert.h>

/*
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
*/

namespace hadron {

Parser::Parser(std::string_view code, std::shared_ptr<ErrorReporter> errorReporter):
    m_lexer(code),
    m_tokenIndex(0),
    m_token(Lexer::Token()),
    m_errorReporter(errorReporter) {}

Parser::~Parser() {}

bool Parser::parse() {
    if (!m_lexer.lex()) {
        return false;
    }
    next();

    m_root = parseRoot();
    while (m_errorReporter->errorCount() == 0 && m_token.type != Lexer::Token::Type::kEmpty) {
        m_root->append(parseRoot());
    }

    return (m_root != nullptr && m_errorReporter->errorCount() == 0);
}

bool Parser::next() {
    if (m_tokenIndex < m_lexer.tokens().size()) {
        m_token = m_lexer.tokens()[m_tokenIndex];
        ++m_tokenIndex;
        return true;
    }
    m_token = Lexer::Token(Lexer::Token::Type::kEmpty, nullptr, 0);
    return false;
}

// Some design conventions around the hand-coded parser:
// Entry conditions are documented with asserts().
// Tail recursion is avoided using the while/node->append pattern.

// root: classes | classextensions | INTERPRET cmdlinecode
// classes: <e> | classes classdef
// classextensions: classextension | classextensions classextension
std::unique_ptr<parse::Node> Parser::parseRoot() {
    switch (m_token.type) {
        case Lexer::Token::Type::kEmpty:
            return std::make_unique<parse::Node>();

        case Lexer::Token::Type::kClassName:
            return parseClass();

        case Lexer::Token::Type::kPlus:
            return parseClassExtension();

        default:
            return parseCmdLineCode();
    }
}

// classdef: classname superclass '{' classvardecls methods '}'
//           | classname '[' optname ']' superclass '{' classvardecls methods '}'
// superclass: <e> | ':' classname
// optname: <e> | name
std::unique_ptr<parse::ClassNode> Parser::parseClass() {
    assert(m_token.type == Lexer::Token::Type::kClassName);
    auto classNode = std::make_unique<parse::ClassNode>(std::string_view(m_token.start, m_token.length));
    next(); // classname
    if (m_token.type == Lexer::Token::Type::kColon) {
        next(); // :
        if (m_token.type != Lexer::Token::Type::kClassName) {
            m_errorReporter->addError(fmt::format("Error parsing class {} at line {}: expecting superclass name after "
                "colon ':'.", classNode->className, m_errorReporter->getLineNumber(m_token.start)));
            return classNode;
        }
        classNode->superClassName = std::string_view(m_token.start, m_token.length);
        next(); // superclass classname
    } else if (m_token.type == Lexer::Token::Type::kOpenSquare) {
        next(); // [
        if (m_token.type != Lexer::Token::Type::kIdentifier) {
            m_errorReporter->addError(fmt::format("Error parsing class {} at line {}: expecting valid optional name "
                "inside square brackets '[' and ']'.", classNode->className,
                m_errorReporter->getLineNumber(m_token.start)));
            return classNode;
        }
        classNode->optionalName = std::string_view(m_token.start, m_token.length);
        next(); // optname
        if (m_token.type != Lexer::Token::Type::kCloseSquare) {
            m_errorReporter->addError(fmt::format("Error parsing class {} at line {}: expecting closing square bracket "
                "']' after optional class name.", classNode->className, m_errorReporter->getLineNumber(m_token.start)));
            return classNode;
        }
        next(); // ]
    }

    if (m_token.type != Lexer::Token::Type::kOpenCurly) {
        m_errorReporter->addError(fmt::format("Error parsing class {} at line {}: expecting opening curly brace '{{'.",
            classNode->className, m_errorReporter->getLineNumber(m_token.start)));
        return classNode;
    }
    auto openCurly = m_token;
    next(); // {

    classNode->variables = parseClassVarDecls();
    classNode->methods = parseMethods();

    if (m_token.type != Lexer::Token::Type::kCloseCurly) {
        m_errorReporter->addError(fmt::format("Error parsing class {} at line {}: expecting closing curly brace '}}' to "
            "match opening brace '{{ on line {}", classNode->className,
            m_errorReporter->getLineNumber(m_token.start), m_errorReporter->getLineNumber(openCurly.start)));
        return classNode;
    }

    next(); // }
    return classNode;
}

// classextension: '+' classname '{' methods '}'
std::unique_ptr<parse::ClassExtNode> Parser::parseClassExtension() {
    assert(m_token.type == Lexer::Token::Type::kPlus);
    next(); // +
    if (m_token.type != Lexer::Token::Type::kClassName) {
        m_errorReporter->addError(fmt::format("Error parsing at line {}: expecting class name after '+' symbol.",
                m_errorReporter->getLineNumber(m_token.start)));
        return std::make_unique<parse::ClassExtNode>(std::string_view());
    }
    auto extension = std::make_unique<parse::ClassExtNode>(std::string_view(m_token.start, m_token.length));
    next(); // classname
    if (m_token.type != Lexer::Token::kOpenCurly) {
        m_errorReporter->addError(fmt::format("Error parsing at line {}: expecting open curly brace '{{' after "
                "class name in class extension.", m_errorReporter->getLineNumber(m_token.start)));
        return extension;
    }
    auto openCurly = m_token;
    next(); // {
    extension->methods = parseMethods();
    if (m_token.type != Lexer::Token::kCloseCurly) {
        m_errorReporter->addError(fmt::format("Error parsing around line {}: expecting closing curly brace '}}' to match "
                "opening brace '{{' on line {}", m_errorReporter->getLineNumber(m_token.start),
                m_errorReporter->getLineNumber(openCurly.start)));
    }
    next(); // }
    return extension;
}

// cmdlinecode: '(' funcvardecls1 funcbody ')'
//              | funcvardecls1 funcbody
//              | funcbody
std::unique_ptr<parse::Node> Parser::parseCmdLineCode() {
    switch (m_token.type) {
    case Lexer::Token::Type::kOpenParen: {
        auto openParenToken = m_token;
        next(); // (
        auto block = std::make_unique<parse::BlockNode>();
        if (m_token.type == Lexer::Token::Type::kVar) {
            block->variables = parseFuncVarDecls();
        }
        block->body = parseFuncBody();
        if (m_token.type != Lexer::Token::kCloseParen) {
            m_errorReporter->addError(fmt::format("Error parsing around line {}: expecting closing parenthesis to "
                    "match opening parenthesis on line {}", m_errorReporter->getLineNumber(m_token.start),
                    m_errorReporter->getLineNumber(openParenToken.start)));
        }
        next(); // )
        return block;
    } break;

    case Lexer::Token::Type::kVar: {
        auto block = std::make_unique<parse::BlockNode>();
        block->variables = parseFuncVarDecls();
        block->body = parseFuncBody();
        return block;
    } break;

    default:
        return parseFuncBody();
    }
}

// classvardecls: <e> | classvardecls classvardecl
std::unique_ptr<parse::VarListNode> Parser::parseClassVarDecls() {
    std::unique_ptr<parse::VarListNode> classVars = parseClassVarDecl();
    if (classVars != nullptr) {
        std::unique_ptr<parse::VarListNode> furtherClassVars = parseClassVarDecl();
        while (furtherClassVars != nullptr) {
            classVars->append(std::move(furtherClassVars));
            furtherClassVars = parseClassVarDecl();
        }
    }
    return classVars;
}

// classvardecl: CLASSVAR rwslotdeflist ';'
//               | VAR rwslotdeflist ';'
//               | SC_CONST constdeflist ';'
std::unique_ptr<parse::VarListNode> Parser::parseClassVarDecl() {
    switch (m_token.type) {
    case Lexer::Token::Type::kClassVar: {
        auto classVars = parseRWVarDefList();
        if (m_token.type != Lexer::Token::kSemicolon) {
            m_errorReporter->addError(fmt::format("Error parsing class variable declaration at line {}, expecting "
                        "semicolon ';'.", m_errorReporter->getLineNumber(m_token.start)));
            return nullptr;
        }
        next(); // ;
        return classVars;
    }

    case Lexer::Token::Type::kVar: {
        auto vars = parseRWVarDefList();
        if (m_token.type != Lexer::Token::kSemicolon) {
            m_errorReporter->addError(fmt::format("Error parsing variable declaration at line {}, expecting "
                        "semicolon ';'.", m_errorReporter->getLineNumber(m_token.start)));
            return nullptr;
        }
        next(); // ;
        return vars;
    }

    case Lexer::Token::Type::kConst: {
        auto constants = parseConstDefList();
        if (m_token.type != Lexer::Token::kSemicolon) {
            m_errorReporter->addError(fmt::format("Error parsing constant declaration at line {}, expecting "
                        "semicolon ';'.", m_errorReporter->getLineNumber(m_token.start)));
            return nullptr;
        }
        next(); // ;
        return constants;
    }

    default:
        return nullptr;
    }
}

// methods: <e> | methods methoddef
// methoddef: name '{' argdecls funcvardecls primitive methbody '}'
//            | binop '{' argdecls funcvardecls primitive methbody '}'
//            | '*' name '{' argdecls funcvardecls primitive methbody '}'
//            | '*' binop '{' argdecls funcvardecls primitive methbody '}'
// TODO: support primitives
std::unique_ptr<parse::MethodNode> Parser::parseMethods() {
    bool isClassMethod = false;
    if (m_token.type == Lexer::Token::Type::kAsterisk) {
        isClassMethod = true;
        next(); // *
    }
    if (m_token.type == Lexer::Token::Type::kIdentifier || m_token.couldBeBinop) {
        auto method = std::make_unique<parse::MethodNode>(std::string_view(m_token.start, m_token.length),
                isClassMethod);
        next(); // name or binop (treated as name)
        if (m_token.type != Lexer::Token::Type::kOpenCurly) {
            m_errorReporter->addError(fmt::format("Error parsing method named '{}' at line {}, expecting opening curly "
                        "brace '{'.", method->methodName, m_errorReporter->getLineNumber(m_token.start)));
            return nullptr;
        }
        next(); // {
        method->arguments = parseArgDecls();
        method->variables = parseFuncVarDecls();
        method->body = parseMethodBody();
        if (m_token.type != Lexer::Token::Type::kCloseCurly) {
            m_errorReporter->addError(fmt::format("Error parsing method named '{}' at line {}, expecting closing curly "
                        "brace '}'.", method->methodName, m_errorReporter->getLineNumber(m_token.start)));
            return nullptr;
        }
        next(); // }
        return method;
    }

    return nullptr;
}

// funcvardecls1: funcvardecl | funcvardecls1 funcvardecl
std::unique_ptr<parse::VarListNode> Parser::parseFuncVarDecls() {
    std::unique_ptr<parse::VarListNode> varDecls;
    if (m_token.type == Lexer::Token::Type::kVar) {
        varDecls = parseFuncVarDecl();
    }
    if (varDecls != nullptr) {
        while (m_token.type == Lexer::Token::Type::kVar) {
            auto furtherVarDecls = parseFuncVarDecl();
            if (furtherVarDecls == nullptr) {
                return nullptr;
            }
            varDecls->append(std::move(furtherVarDecls));
        }
    }
    return varDecls;
}

// funcvardecl: VAR vardeflist ';'
std::unique_ptr<parse::VarListNode> Parser::parseFuncVarDecl() {
    assert(m_token.type == Lexer::Token::Type::kVar);
    next(); // var
    auto varDefList = parseVarDefList();
    if (m_token.type != Lexer::Token::Type::kSemicolon) {
        m_errorReporter->addError(fmt::format("Error parsing variable declaration at line {}, expecting semicolon ';'.",
                    m_errorReporter->getLineNumber(m_token.start)));
        return nullptr;
    }
    next(); // ;
    return varDefList;
}

// funcbody: funretval
//           | exprseq funretval
// funretval: <e> | '^' expr optsemi
std::unique_ptr<parse::Node> Parser::parseFuncBody() {
    if (m_token.type == Lexer::Token::Type::kCaret) {
        next(); // ^
        auto expr = parseExpr();
        if (m_token.type == Lexer::Token::Type::kSemicolon) {
            next(); // ;
        }
        return expr;
    }
    auto exprSeq = parseExprSeq();
    if (m_token.type == Lexer::Token::Type::kCaret) {
        next(); // ^
        exprSeq->append(parseExpr());
        if (m_token.type == Lexer::Token::Type::kSemicolon) {
            next(); // ;
        }
    }
    return exprSeq;
}

// rwslotdeflist: rwslotdef | rwslotdeflist ',' rwslotdef
std::unique_ptr<parse::VarListNode> Parser::parseRWVarDefList() {
    auto varList = std::make_unique<parse::VarListNode>();
    varList->definitions = parseRWVarDef();
    if (varList->definitions != nullptr) {
        while (m_token.type == Lexer::Token::Type::kComma) {
            next(); // ,
            auto nextVarDef = parseRWVarDef();
            if (nextVarDef != nullptr) {
                varList->definitions->append(std::move(nextVarDef));
            } else {
                return nullptr;
            }
        }
    }
    return varList;
}

// rwslotdef: rwspec name | rwspec name '=' slotliteral
// rwspec: <e> | '<' | READWRITEVAR | '>'
std::unique_ptr<parse::VarDefNode> Parser::parseRWVarDef() {
    if (m_token.type == Lexer::Token::Type::kLessThan) {
        next(); // <
    } else if (m_token.type == Lexer::Token::Type::kGreaterThan) {
        next(); // >
    } else if (m_token.type == Lexer::Token::Type::kReadWriteVar) {
        next(); // <>
    }

    if (m_token.type != Lexer::Token::Type::kIdentifier) {
        m_errorReporter->addError(fmt::format("Error parsing class variable declaration at line {}, expecting variable "
                    "name.", m_errorReporter->getLineNumber(m_token.start)));
        return nullptr;
    }

    auto varDef = std::make_unique<parse::VarDefNode>(std::string_view(m_token.start, m_token.length));
    next(); // name
    if (m_token.type == Lexer::Token::Type::kAssign) {
        next(); // =
        varDef->initialValue = parseLiteral();
        if (varDef->initialValue == nullptr) {
            return nullptr;
        }
    }

    return varDef;
}

// constdeflist: constdef | constdeflist optcomma constdef
// optcomma: <e> | ','
std::unique_ptr<parse::VarListNode> Parser::parseConstDefList() {
    auto varList = std::make_unique<parse::VarListNode>();
    varList->definitions = parseConstDef();
    if (varList->definitions == nullptr) {
        m_errorReporter->addError(fmt::format("Error parsing class constant declaration at line {}, expecting constant "
                    "name or read spec character '<'.", m_errorReporter->getLineNumber(m_token.start)));
        return nullptr;
    }
    if (m_token.type == Lexer::Token::Type::kComma) {
        next(); // ,
    }
    auto nextDef = parseConstDef();
    while (nextDef != nullptr) {
        varList->definitions->append(std::move(nextDef));
        if (m_token.type == Lexer::Token::Type::kComma) {
            next(); // ,
        }
        nextDef = parseConstDef();
    }

    return varList;
}

// constdef: rspec name '=' slotliteral
// rspec:  <e> | '<'
std::unique_ptr<parse::VarDefNode> Parser::parseConstDef() {
    if (m_token.type == Lexer::Token::Type::kLessThan) {
        next(); // <
        if (m_token.type != Lexer::Token::Type::kIdentifier) {
            m_errorReporter->addError(fmt::format("Error parsing class constant declaration at line {}, expecting "
                        "constant name after read spec character '<'.", m_errorReporter->getLineNumber(m_token.start)));
            return nullptr;
        }
    } else if (m_token.type != Lexer::Token::Type::kIdentifier) {
        // May not be an error, in this case, may just be the end of the constant list.
        return nullptr;
    }
    auto varDef = std::make_unique<parse::VarDefNode>(std::string_view(m_token.start, m_token.length));
    next(); // name
    if (m_token.type != Lexer::Token::Type::kAssign) {
        m_errorReporter->addError(fmt::format("Error parsing class constant '{}' declaration at line {}, expecting "
                    "assignment operator '='.", varDef->varName, m_errorReporter->getLineNumber(m_token.start)));
        return nullptr;
    }
    next(); // =
    varDef->initialValue = parseLiteral();
    if (varDef->initialValue == nullptr) {
        return nullptr;
    }

    return varDef;
}

// vardeflist: vardef | vardeflist ',' vardef
std::unique_ptr<parse::VarListNode> Parser::parseVarDefList() {
    auto varList = std::make_unique<parse::VarListNode>();
    varList->definitions = parseVarDef();
    if (varList->definitions == nullptr) {
        return nullptr;
    }
    while (m_token.type == Lexer::Token::Type::kComma) {
        next(); // ,
        auto nextVarDef = parseVarDef();
        if (nextVarDef == nullptr) {
            return nullptr;
        }
        varList->definitions->append(std::move(nextVarDef));
    }
    return varList;
}

// vardef: name | name '=' expr | name '(' exprseq ')'
std::unique_ptr<parse::VarDefNode> Parser::parseVarDef() {
    if (m_token.type != Lexer::Token::kIdentifier) {
        m_errorReporter->addError(fmt::format("Error parsing variable definition at line {}, expecting variable name.",
                    m_errorReporter->getLineNumber(m_token.start)));
        return nullptr;
    }
    auto varDef = std::make_unique<parse::VarDefNode>(std::string_view(m_token.start, m_token.length));
    next(); // name
    if (m_token.type == Lexer::Token::kAssign) {
        next(); // =
        varDef->initialValue = parseExpr();
        if (varDef->initialValue == nullptr) {
            return nullptr;
        }
    } else if (m_token.type == Lexer::Token::kOpenParen) {
        Lexer::Token openParen = m_token;
        next(); // (
        varDef->initialValue = parseExprSeq();
        if (varDef->initialValue == nullptr) {
            return nullptr;
        }
        if (m_token.type != Lexer::Token::kCloseParen) {
            m_errorReporter->addError(fmt::format("Error parsing variable definition for variable '{}' on line {}, "
                        "expecting closing parenthesis ')' to match opening parenthesis '(' on line {}",
                        varDef->varName, m_errorReporter->getLineNumber(m_token.start),
                        m_errorReporter->getLineNumber(openParen.start)));
        }
        next(); // )
    }
    return varDef;
}


// argdecls: <e> | ARG vardeflist ';'
//               | ARG vardeflist0 ELLIPSIS name ';'
//               | '|' slotdeflist '|'
//               | '|' slotdeflist0 ELLIPSIS name '|'
std::unique_ptr<parse::ArgListNode> Parser::parseArgDecls() {
    bool isArg;
    if (m_token.type == Lexer::Token::Type::kArg) {
        isArg = true;
    } else if (m_token.type == Lexer::Token::kPipe) {
        isArg = false;
    } else {
        return nullptr;
    }

    next(); // arg or |
    auto argList = std::make_unique<parse::ArgListNode>();
    argList->varList = parseVarDefList();

    if (m_token.type == Lexer::Token::Type::kEllipses) {
        next(); // ...
        if (m_token.type != Lexer::Token::Type::kIdentifier) {
            m_errorReporter->addError(fmt::format("Error parsing argument list on line {}, expecting name after "
                        "ellipses '...'.", m_errorReporter->getLineNumber(m_token.start)));
            return nullptr;
        }
        argList->varArgsName = std::string_view(m_token.start, m_token.length);
        next(); // name
    }

    if (isArg && m_token.type != Lexer::Token::Type::kSemicolon) {
        m_errorReporter->addError(fmt::format("Error parsing argument list on line {}, expected semicolon ';' at "
                    "end of argument list.", m_errorReporter->getLineNumber(m_token.start)));
        return nullptr;
    } else if (!isArg && m_token.type != Lexer::Token::Type::kPipe) {
        m_errorReporter->addError(fmt::format("Error parsing argument list on line {}, expected matching pipe '|' at "
                    "end of argument list.", m_errorReporter->getLineNumber(m_token.start)));
        return nullptr;
    }
    next(); // ; or |

    return argList;
}

// methbody: retval | exprseq retval
// retval: <e> | '^' expr optsemi
std::unique_ptr<parse::Node> Parser::parseMethodBody() {
    auto exprSeq = parseExprSeq();
    if (m_token.type == Lexer::Token::Type::kCaret) {
        next(); // ^
        auto retVal = std::make_unique<parse::ReturnNode>();
        retVal->valueExpr = parseExpr();
        if (retVal->valueExpr == nullptr) {
            return nullptr;
        }
        if (m_token.type == Lexer::Token::Type::kSemicolon) {
            next(); // ;
        }
        if (exprSeq == nullptr) {
            return retVal;
        }
        exprSeq->append(std::move(retVal));
    }

    return exprSeq;
}

// exprseq: exprn optsemi
// exprn: expr | exprn ';' expr
std::unique_ptr<parse::Node> Parser::parseExprSeq() {
    auto exprSeq = parseExpr();
    if (exprSeq == nullptr) {
        return nullptr;
    }
    while (m_token.type == Lexer::Token::Type::kSemicolon) {
        next(); // ;
        auto nextExpr = parseExpr();
        if (nextExpr != nullptr) {
            exprSeq->append(std::move(nextExpr));
        } else {
            break;
        }
    }
    if (m_token.type == Lexer::Token::Type::kSemicolon) {
        next(); // ;
    }
    return exprSeq;
}

// expr: expr1
//       | valrangexd
//       | valrangeassign
//       | expr '.' '[' arglist1 ']'
//       | expr binop2 adverb expr %prec BINOP
//       | expr '.' name '=' expr
//       | expr1 '[' arglist1 ']' '=' expr
//       | expr '.' '[' arglist1 ']' '=' expr
//
// expr1: | curryarg
//        | msgsend
//        | pseudovar
//        | expr1 '[' arglist1 ']'
//        | valrangex1
//
// curryarg: CURRYARG
//
// msgsend:  expr '.' '(' ')' blocklist
//         | expr '.' '(' keyarglist1 optcomma ')' blocklist
//         | expr '.' name '(' keyarglist1 optcomma ')' blocklist
//         | expr '.' '(' arglist1 optkeyarglist ')' blocklist
//         | expr '.' '(' arglistv1 optkeyarglist ')'
//         | expr '.' name '(' ')' blocklist
//         | expr '.' name '(' arglist1 optkeyarglist ')' blocklist
//         | expr '.' name '(' arglistv1 optkeyarglist ')'
//         | expr '.' name blocklist
//
// pseudovar: PSEUDOVAR
//
// blockliteral: block
//
// block: '{' argdecls funcvardecls funcbody '}'
//        | BEGINCLOSEDFUNC argdecls funcvardecls funcbody '}'    (beginclosedfunc is #{, with possible whitespace
//
// mavars: mavarlist | mavarlist ELLIPSIS name
//
// mavarlist: name | mavarlist ',' name
//
// blocklist1: blocklistitem | blocklist1 blocklistitem
//
// blocklistitem : blockliteral | generator
//
// blocklist: <e> | blocklist1
//
std::unique_ptr<parse::Node> Parser::parseExpr() {
    switch (m_token.type) {
    case Lexer::Token::Type::kClassName: {
        // expr: classname
        // TODO: expr -> expr1 -> msgsend: classname '[' arrayelems ']'
        // TODO: expr -> expr1 -> msgsend: classname blocklist1
        // TODO: expr -> expr1 -> msgsend: classname '(' ')' blocklist
        // TODO: expr -> expr1 -> msgsend: classname '(' keyarglist1 optcomma ')' blocklist
        // TODO: expr -> expr1 -> msgsend: classname '(' arglist1 optkeyarglist ')' blocklist
        // TODO: expr -> expr1 -> msgsend: classname '(' arglistv1 optkeyarglist ')'
        auto classNode = std::make_unique<parse::ValueNode>(parse::ValueNode::Type::kClass);
        next();  // classname
        return classNode;
    } break;

    case Lexer::Token::Type::kIdentifier:
        // expr: name '=' expr
        // expr: name '(' arglist1 optkeyarglist ')' '=' expr
        // expr -> expr1 -> pushname: name
        // expr -> expr1 -> msgsend: name blocklist1
        // expr -> expr1 -> msgsend: name '(' ')' blocklist1
        // expr -> expr1 -> msgsend: name '(' arglist1 optkeyarglist ')' blocklist
        // expr -> expr1 -> msgsend: name '(' arglistv1 optkeyarglist ')'

    case Lexer::Token::Type::kGrave:
        // expr: '`' expr TODO: what does this do?

    case Lexer::Token::Type::kTilde:
        // expr: '~' name '=' expr
        // expr -> expr1: '~' name

    case Lexer::Token::Type::kHash:
        // expr: '#' mavars '=' expr

    case Lexer::Token::Type::kOpenParen:
        // expr -> expr1: '(' exprseq ')'
        // expr -> expr1: '(' valrange2 ')'
        // expr -> expr1: '(' ':' valrange3 ')'
        // expr -> expr1: '(' dictslotlist ')'
        // expr -> expr1 -> msgsend: '(' binop2 ')' blocklist1
        // expr -> expr1 -> msgsend: '(' binop2 ')' '(' ')' blocklist1
        // expr -> expr1 -> msgsend: '(' binop2 ')' '(' arglist1 optkeyarglist ')' blocklist
        // expr -> expr1 -> msgsend: '(' binop2 ')' '(' arglistv1 optkeyarglist ')'

    case Lexer::Token::Type::kOpenSquare:
        // expr -> expr1: '[' arrayelems ']'

    case Lexer::Token::Type::kOpenCurly:
        // expr -> expr1 -> generator: '{' ':' exprseq ',' qual '}'
        // expr -> expr1 -> generator: '{' ';' exprseq  ',' qual '}'

    default:
        return nullptr;
    }
    return nullptr;
}

// slotliteral: integer | floatp | ascii | string | symbol | trueobj | falseobj | nilobj | listlit
// pushliteral: integer | floatp | ascii | string | symbol | trueobj | falseobj | nilobj | listlit
std::unique_ptr<parse::LiteralNode> Parser::parseLiteral() {
    switch (m_token.type) {
    default:
    case Lexer::Token::Type::kInteger:
        // expr -> expr1 -> slotliteral/pushliteral: integer

    case Lexer::Token::Type::kFloat:
        // expr -> expr1 -> slotliteral/pushliteral: floatp

    case Lexer::Token::Type::kString:
        // expr -> expr1 -> slotliteral/pushliteral: string

    case Lexer::Token::Type::kSymbol:
        // expr -> expr1 -> slotliteral/pushliteral: symbol

    case Lexer::Token::Type::kTrue:
        // expr -> expr1 -> slotliteral/pushliteral: trueobj

    case Lexer::Token::Type::kFalse:
        // expr -> expr1 -> slotliteral/pushliteral: falseobj

    case Lexer::Token::Type::kNil:
        // expr -> expr1 -> slotliteral/pushliteral: nilobj

    case Lexer::Token::Type::kHash:  // for listlit
        // expr -> expr1 -> slotliteral/pushliteral: hash

        return nullptr;
    }

    return nullptr;
}

} // namespace hadron
