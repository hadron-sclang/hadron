#include "Parser.hpp"

#include "ErrorReporter.hpp"
#include "Lexer.hpp"
#include "SymbolTable.hpp"

#include "fmt/core.h"
#include "spdlog/spdlog.h"

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

methods: <e> | methods miethoddef

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
    m_errorReporter(errorReporter) {
    m_errorReporter->setCode(code.data());
}

Parser::Parser(std::string_view code):
    m_lexer(code),
    m_tokenIndex(0),
    m_token(Lexer::Token()) {
    m_errorReporter = std::make_shared<ErrorReporter>(true);
    m_errorReporter->setCode(code.data());
}

Parser::~Parser() {}

bool Parser::parse() {
    if (!m_lexer.lex(m_errorReporter.get())) {
        return false;
    }

    if (m_lexer.tokens().size() > 0) {
        m_token = m_lexer.tokens()[0];
    } else {
        m_token = Lexer::Token(Lexer::Token::Name::kEmpty, nullptr, 0);
    }

    m_root = parseRoot();
    if (!m_root) return false;

    while (m_errorReporter->errorCount() == 0 && m_token.name != Lexer::Token::Name::kEmpty) {
        m_root->append(parseRoot());
    }

    return (m_root != nullptr && m_errorReporter->errorCount() == 0);
}

bool Parser::next() {
    ++m_tokenIndex;
    if (m_tokenIndex < m_lexer.tokens().size()) {
        m_token = m_lexer.tokens()[m_tokenIndex];
        return true;
    }
    m_token = Lexer::Token(Lexer::Token::Name::kEmpty, nullptr, 0);
    return false;
}

// Some design conventions around the hand-coded parser:
// Entry conditions are documented with asserts().
// Tail recursion is avoided using the while/node->append pattern.

// root: classes | classextensions | cmdlinecode
// classes: <e> | classes classdef
// classextensions: classextension | classextensions classextension
std::unique_ptr<parse::Node> Parser::parseRoot() {
    switch (m_token.name) {
        case Lexer::Token::Name::kEmpty:
            return std::make_unique<parse::Node>(parse::NodeType::kEmpty, m_tokenIndex);

        case Lexer::Token::Name::kClassName:
            return parseClass();

        case Lexer::Token::Name::kPlus:
            return parseClassExtension();

        default:
            return parseCmdLineCode();
    }
}

// classdef: classname superclass '{' classvardecls methods '}'
//         | classname '[' optname ']' superclass '{' classvardecls methods '}'
// superclass: <e> | ':' classname
// optname: <e> | name
std::unique_ptr<parse::ClassNode> Parser::parseClass() {
    assert(m_token.name == Lexer::Token::Name::kClassName);
    auto classNode = std::make_unique<parse::ClassNode>(m_tokenIndex);
    auto className = m_token;
    next(); // classname

    if (m_token.name == Lexer::Token::Name::kOpenSquare) {
        next(); // [
        if (m_token.name != Lexer::Token::Name::kIdentifier) {
            m_errorReporter->addError(fmt::format("Error parsing class {} at line {}: expecting valid optional name "
                "inside square brackets '[' and ']'.", className.range,
                m_errorReporter->getLineNumber(m_token.range.data())));
            return classNode;
        }
        classNode->optionalNameIndex = m_tokenIndex;
        next(); // optname
        if (m_token.name != Lexer::Token::Name::kCloseSquare) {
            m_errorReporter->addError(fmt::format("Error parsing class {} at line {}: expecting closing square bracket "
                "']' after optional class name.", className.range, m_errorReporter->getLineNumber(
                    m_token.range.data())));
            return classNode;
        }
        next(); // ]
    }

    if (m_token.name == Lexer::Token::Name::kColon) {
        next(); // :
        if (m_token.name != Lexer::Token::Name::kClassName) {
            m_errorReporter->addError(fmt::format("Error parsing class {} at line {}: expecting superclass name after "
                "colon ':'.", className.range, m_errorReporter->getLineNumber(m_token.range.data())));
            return classNode;
        }
        classNode->superClassNameIndex = m_tokenIndex;
        next(); // superclass classname
    }

    if (m_token.name != Lexer::Token::Name::kOpenCurly) {
        m_errorReporter->addError(fmt::format("Error parsing class {} at line {}: expecting opening curly brace '{{'.",
            className.range, m_errorReporter->getLineNumber(m_token.range.data())));
        return classNode;
    }
    auto openCurly = m_token;
    next(); // {

    classNode->variables = parseClassVarDecls();
    classNode->methods = parseMethods();

    if (m_token.name != Lexer::Token::Name::kCloseCurly) {
        m_errorReporter->addError(fmt::format("Error parsing class {} at line {}: expecting closing curly brace '}}' "
            "to match opening brace '{{' on line {}", className.range,
            m_errorReporter->getLineNumber(m_token.range.data()), m_errorReporter->getLineNumber(
                openCurly.range.data())));
        return classNode;
    }

    next(); // }
    return classNode;
}

// classextension: '+' classname '{' methods '}'
std::unique_ptr<parse::ClassExtNode> Parser::parseClassExtension() {
    assert(m_token.name == Lexer::Token::Name::kPlus);
    next(); // +
    if (m_token.name != Lexer::Token::Name::kClassName) {
        m_errorReporter->addError(fmt::format("Error parsing at line {}: expecting class name after '+' symbol.",
                m_errorReporter->getLineNumber(m_token.range.data())));
        return std::make_unique<parse::ClassExtNode>(m_tokenIndex);
    }
    auto extension = std::make_unique<parse::ClassExtNode>(m_tokenIndex);
    next(); // classname
    if (m_token.name != Lexer::Token::kOpenCurly) {
        m_errorReporter->addError(fmt::format("Error parsing at line {}: expecting open curly brace '{{' after "
                "class name in class extension.", m_errorReporter->getLineNumber(m_token.range.data())));
        return extension;
    }
    auto openCurly = m_token;
    next(); // {
    extension->methods = parseMethods();
    if (m_token.name != Lexer::Token::kCloseCurly) {
        m_errorReporter->addError(fmt::format("Error parsing around line {}: expecting closing curly brace '}}' to "
                "match opening brace '{{' on line {}", m_errorReporter->getLineNumber(m_token.range.data()),
                m_errorReporter->getLineNumber(openCurly.range.data())));
    }
    next(); // }
    return extension;
}

// cmdlinecode: '(' funcvardecls1 funcbody ')'
//            | funcvardecls1 funcbody
//            | funcbody
std::unique_ptr<parse::Node> Parser::parseCmdLineCode() {
    switch (m_token.name) {
    case Lexer::Token::Name::kOpenParen: {
        auto openParenToken = m_token;
        auto block = std::make_unique<parse::BlockNode>(m_tokenIndex);
        next(); // (
        block->variables = parseFuncVarDecls();
        block->body = parseFuncBody();
        if (m_token.name != Lexer::Token::kCloseParen) {
            m_errorReporter->addError(fmt::format("Error parsing around line {}: expecting closing parenthesis to "
                    "match opening parenthesis on line {}", m_errorReporter->getLineNumber(m_token.range.data()),
                    m_errorReporter->getLineNumber(openParenToken.range.data())));
        }
        next(); // )
        return block;
    } break;

    case Lexer::Token::Name::kVar: {
        auto block = std::make_unique<parse::BlockNode>(m_tokenIndex);
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
    auto classVars = parseClassVarDecl();
    if (classVars != nullptr) {
        auto furtherClassVars = parseClassVarDecl();
        while (furtherClassVars != nullptr) {
            classVars->append(std::move(furtherClassVars));
            furtherClassVars = parseClassVarDecl();
        }
    }
    return classVars;
}

// classvardecl: CLASSVAR rwslotdeflist ';'
//             | VAR rwslotdeflist ';'
//             | SC_CONST constdeflist ';'
std::unique_ptr<parse::VarListNode> Parser::parseClassVarDecl() {
    switch (m_token.name) {
    case Lexer::Token::Name::kClassVar: {
        auto classVars = parseRWVarDefList();
        if (m_token.name != Lexer::Token::kSemicolon) {
            m_errorReporter->addError(fmt::format("Error parsing class variable declaration at line {}, expecting "
                        "semicolon ';'.", m_errorReporter->getLineNumber(m_token.range.data())));
            return nullptr;
        }
        next(); // ;
        return classVars;
    }

    case Lexer::Token::Name::kVar: {
        auto vars = parseRWVarDefList();
        if (m_token.name != Lexer::Token::kSemicolon) {
            m_errorReporter->addError(fmt::format("Error parsing variable declaration at line {}, expecting "
                        "semicolon ';'.", m_errorReporter->getLineNumber(m_token.range.data())));
            return nullptr;
        }
        next(); // ;
        return vars;
    }

    case Lexer::Token::Name::kConst: {
        auto constants = parseConstDefList();
        if (m_token.name != Lexer::Token::kSemicolon) {
            m_errorReporter->addError(fmt::format("Error parsing constant declaration at line {}, expecting "
                        "semicolon ';'.", m_errorReporter->getLineNumber(m_token.range.data())));
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
std::unique_ptr<parse::MethodNode> Parser::parseMethods() {
    auto method = parseMethod();
    if (method != nullptr) {
        auto nextMethod = parseMethod();
        while (nextMethod != nullptr) {
            method->append(std::move(nextMethod));
            nextMethod = parseMethod();
        }
    }
    return method;
}

// methoddef: name '{' argdecls funcvardecls primitive methbody '}'
//          | binop '{' argdecls funcvardecls primitive methbody '}'
//          | '*' name '{' argdecls funcvardecls primitive methbody '}'
//          | '*' binop '{' argdecls funcvardecls primitive methbody '}'
// primitive: <e> | primname optsemi
// primname: PRIMITIVENAME
std::unique_ptr<parse::MethodNode> Parser::parseMethod() {
    bool isClassMethod = false;

    if (m_token.name == Lexer::Token::Name::kAsterisk) {
        isClassMethod = true;
        next(); // *
    }
    std::unique_ptr<parse::MethodNode> method;
    // Special case for binop instance method named '*'
    if (m_token.name == Lexer::Token::Name::kOpenCurly) {
        isClassMethod = false;
        method = std::make_unique<parse::MethodNode>(m_tokenIndex - 1, isClassMethod);
    } else if (m_token.name == Lexer::Token::Name::kIdentifier || m_token.couldBeBinop) {
        method = std::make_unique<parse::MethodNode>(m_tokenIndex, isClassMethod);
        next(); // name or binop (treated as name)
    } else {
        return nullptr;
    }

    if (m_token.name != Lexer::Token::Name::kOpenCurly) {
        m_errorReporter->addError(fmt::format("Error parsing method named '{}' at line {}, expecting opening curly "
                    "brace '{{'.", m_lexer.tokens()[method->tokenIndex].range,
                        m_errorReporter->getLineNumber(m_token.range.data())));
        return nullptr;
    }
    next(); // {
    method->arguments = parseArgDecls();
    method->variables = parseFuncVarDecls();

    if (m_token.name == Lexer::Token::Name::kPrimitive) {
        method->primitiveIndex = m_tokenIndex;
        next(); // primitive
        if (m_token.name == Lexer::Token::Name::kSemicolon) {
            next(); // optsemi
        }
    }

    method->body = parseMethodBody();
    if (m_token.name != Lexer::Token::Name::kCloseCurly) {
        m_errorReporter->addError(fmt::format("Error parsing method named '{}' at line {}, expecting closing curly "
                    "brace '}}'.", m_lexer.tokens()[method->tokenIndex].range,
                    m_errorReporter->getLineNumber(m_token.range.data())));
        return nullptr;
    }
    next(); // }
    return method;
}

// funcvardecls1: funcvardecl | funcvardecls1 funcvardecl
std::unique_ptr<parse::VarListNode> Parser::parseFuncVarDecls() {
    std::unique_ptr<parse::VarListNode> varDecls;
    if (m_token.name == Lexer::Token::Name::kVar) {
        varDecls = parseFuncVarDecl();
    }
    if (varDecls != nullptr) {
        while (m_token.name == Lexer::Token::Name::kVar) {
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
    assert(m_token.name == Lexer::Token::Name::kVar);
    auto varDefList = parseVarDefList();
    if (m_token.name != Lexer::Token::Name::kSemicolon) {
        m_errorReporter->addError(fmt::format("Error parsing variable declaration at line {}, expecting semicolon ';'.",
                    m_errorReporter->getLineNumber(m_token.range.data())));
        return nullptr;
    }
    next(); // ;
    return varDefList;
}

// funcbody: funretval
//         | exprseq funretval
// funretval: <e> | '^' expr optsemi
std::unique_ptr<parse::Node> Parser::parseFuncBody() {
    if (m_token.name == Lexer::Token::Name::kCaret) {
        auto retNode = std::make_unique<parse::ReturnNode>(m_tokenIndex);
        next(); // ^
        retNode->valueExpr = parseExpr();
        if (m_token.name == Lexer::Token::Name::kSemicolon) {
            next(); // ;
        }
        return retNode;
    }
    size_t bodyStart = m_tokenIndex;
    auto prefix = parseExprSeq();
    if (!prefix) return nullptr;

    if (m_token.name == Lexer::Token::Name::kCaret) {
        auto retNode = std::make_unique<parse::ReturnNode>(m_tokenIndex);
        next(); // ^
        retNode->valueExpr = parseExpr();
        if (m_token.name == Lexer::Token::Name::kSemicolon) {
            next(); // ;
        }
        if (prefix->nodeType == parse::NodeType::kExprSeq) {
            auto exprSeq = reinterpret_cast<parse::ExprSeqNode*>(prefix.get());
            exprSeq->expr->append(std::move(retNode));
        } else {
            auto exprSeq = std::make_unique<parse::ExprSeqNode>(bodyStart, std::move(prefix));
            exprSeq->expr->append(std::move(retNode));
            return exprSeq;
        }
    }
    return prefix;
}

// rwslotdeflist: rwslotdef | rwslotdeflist ',' rwslotdef
std::unique_ptr<parse::VarListNode> Parser::parseRWVarDefList() {
    assert(m_token.name == Lexer::Token::Name::kVar || m_token.name == Lexer::Token::Name::kClassVar);
    auto varList = std::make_unique<parse::VarListNode>(m_tokenIndex);
    next(); // var or classvar
    varList->definitions = parseRWVarDef();
    if (varList->definitions != nullptr) {
        while (m_token.name == Lexer::Token::Name::kComma) {
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
    bool readAccess = false;
    bool writeAccess = false;

    if (m_token.name == Lexer::Token::Name::kLessThan) {
        readAccess = true;
        next(); // <
    } else if (m_token.name == Lexer::Token::Name::kGreaterThan) {
        writeAccess = true;
        next(); // >
    } else if (m_token.name == Lexer::Token::Name::kReadWriteVar) {
        readAccess = true;
        writeAccess = true;
        next(); // <>
    }

    if (m_token.name != Lexer::Token::Name::kIdentifier) {
        m_errorReporter->addError(fmt::format("Error parsing class variable declaration at line {}, expecting variable "
                    "name.", m_errorReporter->getLineNumber(m_token.range.data())));
        return nullptr;
    }

    auto varDef = std::make_unique<parse::VarDefNode>(m_tokenIndex);
    next(); // name

    varDef->hasReadAccessor = readAccess;
    varDef->hasWriteAccessor = writeAccess;

    if (m_token.name == Lexer::Token::Name::kAssign) {
        next(); // =
        varDef->initialValue = parseLiteral();
        if (varDef->initialValue == nullptr) {
            m_errorReporter->addError(fmt::format("Error parsing class variable declaration at line {}, expecting "
                    "literal (e.g. number, string, symbol) following assignment.",
                    m_errorReporter->getLineNumber(m_token.range.data())));
            return nullptr;
        }
    }

    return varDef;
}

// constdeflist: constdef | constdeflist optcomma constdef
// optcomma: <e> | ','
std::unique_ptr<parse::VarListNode> Parser::parseConstDefList() {
    assert(m_token.name == Lexer::Token::Name::kConst);
    auto varList = std::make_unique<parse::VarListNode>(m_tokenIndex);
    next(); // const
    varList->definitions = parseConstDef();
    if (varList->definitions == nullptr) {
        m_errorReporter->addError(fmt::format("Error parsing class constant declaration at line {}, expecting constant "
                    "name or read spec character '<'.", m_errorReporter->getLineNumber(m_token.range.data())));
        return nullptr;
    }
    if (m_token.name == Lexer::Token::Name::kComma) {
        next(); // ,
    }
    auto nextDef = parseConstDef();
    while (nextDef != nullptr) {
        varList->definitions->append(std::move(nextDef));
        if (m_token.name == Lexer::Token::Name::kComma) {
            next(); // ,
        }
        nextDef = parseConstDef();
    }

    return varList;
}

// constdef: rspec name '=' slotliteral
// rspec:  <e> | '<'
std::unique_ptr<parse::VarDefNode> Parser::parseConstDef() {
    bool readAccess = false;
    if (m_token.name == Lexer::Token::Name::kLessThan) {
        readAccess = true;
        next(); // <
        if (m_token.name != Lexer::Token::Name::kIdentifier) {
            m_errorReporter->addError(fmt::format("Error parsing class constant declaration at line {}, expecting "
                        "constant name after read spec character '<'.", m_errorReporter->getLineNumber(
                            m_token.range.data())));
            return nullptr;
        }
    } else if (m_token.name != Lexer::Token::Name::kIdentifier) {
        // May not be an error, in this case, may just be the end of the constant list.
        return nullptr;
    }
    auto varDef = std::make_unique<parse::VarDefNode>(m_tokenIndex);
    next(); // name
    varDef->hasReadAccessor = readAccess;
    if (m_token.name != Lexer::Token::Name::kAssign) {
        m_errorReporter->addError(fmt::format("Error parsing class constant '{}' declaration at line {}, expecting "
                    "assignment operator '='.", m_lexer.tokens()[varDef->tokenIndex].range,
                    m_errorReporter->getLineNumber(m_token.range.data())));
        return nullptr;
    }
    next(); // =
    varDef->initialValue = parseLiteral();
    if (varDef->initialValue == nullptr) {
        m_errorReporter->addError(fmt::format("Error parsing class constant '{}' declaration at line {}, expecting "
                    "literal (e.g. number, string, symbol) following assignment.",
                    m_lexer.tokens()[varDef->tokenIndex].range,
                    m_errorReporter->getLineNumber(m_token.range.data())));
        return nullptr;
    }
    return varDef;
}

// vardeflist: vardef | vardeflist ',' vardef
std::unique_ptr<parse::VarListNode> Parser::parseVarDefList() {
    auto varList = std::make_unique<parse::VarListNode>(m_tokenIndex);
    if (m_token.name == Lexer::Token::Name::kVar) {
        next(); // var
    }
    varList->definitions = parseVarDef();
    if (varList->definitions == nullptr) {
        return nullptr;
    }
    while (m_token.name == Lexer::Token::Name::kComma) {
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
    if (m_token.name != Lexer::Token::kIdentifier) {
        m_errorReporter->addError(fmt::format("Error parsing variable definition at line {}, expecting variable name.",
                    m_errorReporter->getLineNumber(m_token.range.data())));
        return nullptr;
    }
    auto varDef = std::make_unique<parse::VarDefNode>(m_tokenIndex);
    next(); // name
    if (m_token.name == Lexer::Token::kAssign) {
        next(); // =
        varDef->initialValue = parseExpr();
        if (varDef->initialValue == nullptr) {
            return nullptr;
        }
    } else if (m_token.name == Lexer::Token::kOpenParen) {
        Lexer::Token openParen = m_token;
        next(); // (
        varDef->initialValue = parseExprSeq();
        if (varDef->initialValue == nullptr) {
            return nullptr;
        }
        if (m_token.name != Lexer::Token::kCloseParen) {
            m_errorReporter->addError(fmt::format("Error parsing variable definition for variable '{}' on line {}, "
                        "expecting closing parenthesis ')' to match opening parenthesis '(' on line {}",
                        m_lexer.tokens()[varDef->tokenIndex].range,
                        m_errorReporter->getLineNumber(m_token.range.data()),
                        m_errorReporter->getLineNumber(openParen.range.data())));
        }
        next(); // )
    }
    return varDef;
}

// slotdeflist0: <e> | slotdeflist
// slotdeflist: slotdef
//            | slotdeflist optcomma slotdef
std::unique_ptr<parse::VarListNode> Parser::parseSlotDefList() {
    auto varList = std::make_unique<parse::VarListNode>(m_tokenIndex);
    varList->definitions = parseSlotDef();
    if (varList->definitions == nullptr) {
        return nullptr;
    }
    while (m_token.name == Lexer::Token::Name::kComma) {
        next(); // ,
        auto nextVarDef = parseSlotDef();
        if (nextVarDef == nullptr) {
            return nullptr;
        }
        varList->definitions->append(std::move(nextVarDef));
    }
    return varList;

}

// slotdef: name
//        | name optequal slotliteral
//        | name optequal '(' exprseq ')'
std::unique_ptr<parse::VarDefNode> Parser::parseSlotDef() {
    if (m_token.name != Lexer::Token::kIdentifier) {
        m_errorReporter->addError(fmt::format("Error parsing argument definition at line {}, expecting argument name.",
                    m_errorReporter->getLineNumber(m_token.range.data())));
        return nullptr;
    }
    auto varDef = std::make_unique<parse::VarDefNode>(m_tokenIndex);
    next(); // name
    if (m_token.name == Lexer::Token::kAssign) {
        next(); // =
        varDef->initialValue = parseLiteral();
        if (varDef->initialValue == nullptr) {
            return nullptr;
        }
    } else if (m_token.name == Lexer::Token::kOpenParen) {
        Lexer::Token openParen = m_token;
        next(); // (
        varDef->initialValue = parseExprSeq();
        if (varDef->initialValue == nullptr) {
            return nullptr;
        }
        if (m_token.name != Lexer::Token::kCloseParen) {
            m_errorReporter->addError(fmt::format("Error parsing variable definition for variable '{}' on line {}, "
                        "expecting closing parenthesis ')' to match opening parenthesis '(' on line {}",
                        m_lexer.tokens()[varDef->tokenIndex].range,
                        m_errorReporter->getLineNumber(m_token.range.data()),
                        m_errorReporter->getLineNumber(openParen.range.data())));
        }
        next(); // )
    }
    return varDef;
}

// Pipe argument lists have tighter requirements, slotdeflist vs. vardeflist, for initial values because the pipe
// terminating the argument list could be mistaken for a binop and so results in an ambiguous parse

// argdecls: <e>
//         | ARG vardeflist ';'
//         | ARG vardeflist0 ELLIPSIS name ';'
//         | '|' slotdeflist '|'
//         | '|' slotdeflist0 ELLIPSIS name '|'
std::unique_ptr<parse::ArgListNode> Parser::parseArgDecls() {
    bool isArg;
    std::unique_ptr<parse::ArgListNode> argList;
    if (m_token.name == Lexer::Token::Name::kArg) {
        isArg = true;
        argList = std::make_unique<parse::ArgListNode>(m_tokenIndex);
        next(); // arg
        if (m_token.name != Lexer::Token::Name::kEllipses) {
            argList->varList = parseVarDefList();
        }
    } else if (m_token.name == Lexer::Token::kPipe) {
        isArg = false;
        argList = std::make_unique<parse::ArgListNode>(m_tokenIndex);
        next(); // |
        if (m_token.name != Lexer::Token::Name::kEllipses) {
            argList->varList = parseSlotDefList();
        }
    } else {
        return nullptr;
    }

    if (m_token.name == Lexer::Token::Name::kEllipses) {
        next(); // ...
        if (m_token.name != Lexer::Token::Name::kIdentifier) {
            m_errorReporter->addError(fmt::format("Error parsing argument list on line {}, expecting name after "
                        "ellipses '...'.", m_errorReporter->getLineNumber(m_token.range.data())));
            return nullptr;
        }
        argList->varArgsNameIndex = m_tokenIndex;
        next(); // name
    }

    if (isArg && m_token.name != Lexer::Token::Name::kSemicolon) {
        m_errorReporter->addError(fmt::format("Error parsing argument list on line {}, expected semicolon ';' at "
                    "end of argument list.", m_errorReporter->getLineNumber(m_token.range.data())));
        return nullptr;
    } else if (!isArg && m_token.name != Lexer::Token::Name::kPipe) {
        m_errorReporter->addError(fmt::format("Error parsing argument list on line {}, expected matching pipe '|' at "
                    "end of argument list.", m_errorReporter->getLineNumber(m_token.range.data())));
        return nullptr;
    }
    next(); // ; or |

    return argList;
}

// methbody: retval | exprseq retval
// retval: <e> | '^' expr optsemi
std::unique_ptr<parse::Node> Parser::parseMethodBody() {
    size_t bodyIndex = m_tokenIndex;
    auto prefix = parseExprSeq();

    if (m_token.name == Lexer::Token::Name::kCaret) {
        auto retVal = std::make_unique<parse::ReturnNode>(m_tokenIndex);
        next(); // ^
        retVal->valueExpr = parseExpr();
        if (retVal->valueExpr == nullptr) {
            return nullptr;
        }
        if (m_token.name == Lexer::Token::Name::kSemicolon) {
            next(); // ;
        }

        if (prefix == nullptr) {
            return retVal;
        }

        // If we had already parsed an expression sequence we can append the return value to the existing sequence.
        if (prefix->nodeType == parse::NodeType::kExprSeq) {
            auto exprSeq = reinterpret_cast<parse::ExprSeqNode*>(prefix.get());
            exprSeq->expr->append(std::move(retVal));
        } else {
            // This case is a single expression was parsed, so the returned node from parseExprSeq() is not an
            // ExprSeqNode. Construct a new ExprSeqNode with the prefix as the first expr and append the retval to that.
            auto exprSeq = std::make_unique<parse::ExprSeqNode>(bodyIndex, std::move(prefix));
            exprSeq->expr->append(std::move(retVal));
            return exprSeq;
        }
    }

    return prefix;
}

// exprseq: exprn optsemi
// exprn: expr | exprn ';' expr
std::unique_ptr<parse::Node> Parser::parseExprSeq() {
    size_t startIndex = m_tokenIndex;
    auto expr = parseExpr();
    if (!expr) return nullptr;

    if (m_token.name == Lexer::Token::Name::kSemicolon) {
        next(); // ;
        auto secondExpr = parseExpr();
        if (secondExpr) {
            auto exprSeq = std::make_unique<parse::ExprSeqNode>(startIndex, std::move(expr));
            exprSeq->expr->append(std::move(secondExpr));
            while (m_token.name == Lexer::Token::Name::kSemicolon) {
                next(); // ;
                auto nextExpr = parseExpr();
                if (nextExpr) {
                    exprSeq->expr->append(std::move(nextExpr));
                } else {
                    return exprSeq;
                }
            }
            return exprSeq;
        }
    }

    return expr;
}

// expr: expr1
//     | valrangexd
//     | valrangeassign
//
// expr1: blockliteral
//      | curryarg
//      | msgsend
//      | pseudovar
//      | valrangex1
//
// curryarg: CURRYARG
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
    std::unique_ptr<parse::Node> expr;
    bool isSingleExpression = false;

    switch (m_token.name) {
    case Lexer::Token::Name::kClassName: {
        next();  // classname
        if (m_token.name == Lexer::Token::kOpenSquare) {
            // expr -> expr1 -> msgsend: classname '[' arrayelems ']'
            auto dynList = std::make_unique<parse::DynListNode>(m_tokenIndex - 1);
            Lexer::Token openSquare = m_token;
            next(); // [
            dynList->elements = parseArrayElements();
            if (m_token.name != Lexer::Token::kCloseSquare) {
                m_errorReporter->addError(fmt::format("Error parsing dynamic list on line {}, expecting closing square "
                        "bracket ']' to match opening square bracket '[' on line {}",
                        m_errorReporter->getLineNumber(m_token.range.data()),
                        m_errorReporter->getLineNumber(openSquare.range.data())));
            }
            next(); // ]
            expr = std::move(dynList);
            isSingleExpression = true;
        } else if (m_token.name == Lexer::Token::kOpenParen) {
            // TODO: expr -> expr1 -> msgsend: classname '(' ')' blocklist
            // TODO: expr -> expr1 -> msgsend: classname '(' keyarglist1 optcomma ')' blocklist
            // TODO: expr -> expr1 -> msgsend: classname '(' arglist1 optkeyarglist ')' blocklist
            // TODO: expr -> expr1 -> msgsend: classname '(' arglistv1 optkeyarglist ')'
        } else if (m_token.name == Lexer::Token::kOpenCurly) {
            // TODO: expr -> expr1 -> msgsend: classname blocklist1
        } else {
            // expr: classname
            expr = std::make_unique<parse::NameNode>(m_tokenIndex - 1);
        }
    } break;

    case Lexer::Token::Name::kIdentifier: {
        size_t nameIndex = m_tokenIndex;
        next(); // name
        if (m_token.name == Lexer::Token::kAssign) {
            // expr: name '=' expr
            auto assign = std::make_unique<parse::AssignNode>(m_tokenIndex - 1);
            next(); // =
            assign->name = std::make_unique<parse::NameNode>(nameIndex);
            assign->value = parseExpr();
            expr = std::move(assign);
        } else if (m_token.name == Lexer::Token::kOpenParen) {
            // TODO: expr: name '(' arglist1 optkeyarglist ')' '=' expr
            // TODO: expr -> expr1 -> msgsend: name '(' ')' blocklist1
            // TODO: expr -> expr1 -> msgsend: name '(' arglist1 optkeyarglist ')' blocklist
            // TODO: expr -> expr1 -> msgsend: name '(' arglistv1 optkeyarglist ')'
        } else {
            auto blockList = parseBlockList();
            if (blockList) {
                // expr -> expr1 -> msgsend: name blocklist1
                auto call = std::make_unique<parse::CallNode>(nameIndex);
                call->arguments = std::move(blockList);
                expr = std::move(call);
            } else {
                // expr -> expr1 -> pushname: name
                expr = std::make_unique<parse::NameNode>(nameIndex);
                isSingleExpression = true;
            }
        }

    } break;

    case Lexer::Token::Name::kGrave:
        // expr: '`' expr
        break;

    case Lexer::Token::Name::kTilde: {
        // expr: '~' name '=' expr
        // expr -> expr1: '~' name
        next(); // ~
        if (m_token.name == Lexer::Token::kIdentifier) {
            auto name = std::make_unique<parse::NameNode>(m_tokenIndex);
            name->isGlobal = true;
            next(); // name
            if (m_token.name == Lexer::Token::kAssign) {
                auto assign = std::make_unique<parse::AssignNode>(m_tokenIndex - 1);
                next();
                assign->name = std::move(name);
                assign->value = parseExpr();
                expr = std::move(assign);
            } else {
                expr = std::move(name);
                isSingleExpression = true;
            }
        } else {
            m_errorReporter->addError(fmt::format("Error parsing code on line {}, expected variable name after global "
                    "symbol '~'.", m_errorReporter->getLineNumber(m_token.range.data())));
            return nullptr;
        }
    } break;

    case Lexer::Token::Name::kHash:
        // expr: '#' mavars '=' expr
        break;

    // valrange2: exprseq DOTDOT
    //          | DOTDOT exprseq
    //          | exprseq DOTDOT exprseq
    //          | exprseq ',' exprseq DOTDOT exprseq
    //          | exprseq ',' exprseq DOTDOT
    case Lexer::Token::Name::kOpenParen: {
        isSingleExpression = true;
        Lexer::Token openParen = m_token;
        // expr -> expr1: '(' exprseq ')'
        next(); // (
        expr = parseExprSeq();
        if (m_token.name != Lexer::Token::kCloseParen) {
            m_errorReporter->addError(fmt::format("Error parsing expression sequence on line {}, expecting closing "
                    "parenthesis ')' to match opening parenthesis '(' on line {}",
                    m_errorReporter->getLineNumber(m_token.range.data()),
                    m_errorReporter->getLineNumber(openParen.range.data())));
            return nullptr;
        }
        next(); // )
        // expr -> expr1: '(' valrange2 ')'
        // expr -> expr1: '(' ':' valrange3 ')'
        // expr -> expr1: '(' dictslotlist ')'
        // dictslotdef: exprseq ':' exprseq | keybinop exprseq
        // expr -> expr1 -> msgsend: '(' binop2 ')' blocklist1
        // expr -> expr1 -> msgsend: '(' binop2 ')' '(' ')' blocklist1
        // expr -> expr1 -> msgsend: '(' binop2 ')' '(' arglist1 optkeyarglist ')' blocklist
        // expr -> expr1 -> msgsend: '(' binop2 ')' '(' arglistv1 optkeyarglist ')'
    } break;

    case Lexer::Token::Name::kOpenSquare: {
        // expr -> expr1: '[' arrayelems ']'
        auto dynList = std::make_unique<parse::DynListNode>(m_tokenIndex);
        Lexer::Token openSquare = m_token;
        next(); // [
        dynList->elements = parseArrayElements();
        if (m_token.name != Lexer::Token::kCloseSquare) {
            m_errorReporter->addError(fmt::format("Error parsing dynamic list on line {}, expecting closing square "
                    "bracket ']' to match opening square bracket '[' on line {}",
                    m_errorReporter->getLineNumber(m_token.range.data()),
                    m_errorReporter->getLineNumber(openSquare.range.data())));
                    return nullptr;
        }
        next(); // ]
        expr = std::move(dynList);
        isSingleExpression = true;
    } break;

    case Lexer::Token::Name::kOpenCurly: {
        // expr -> expr1 -> generator: '{' ':' exprseq ',' qual '}'
        // expr -> expr1 -> generator: '{' ';' exprseq  ',' qual '}'
        // expr -> expr1 -> blockliteral -> block -> '{' argdecls funcvardecls funcbody '}'
        // expr -> expr1 -> blockliteral -> block -> BEGINCLOSEDFUNC argdecls funcvardecls funcbody '}'
        Lexer::Token openCurly = m_token;
        next(); // {
        if (m_token.name == Lexer::Token::Name::kColon || m_token.name == Lexer::Token::Name::kSemicolon) {
            // Generator expressions parse here.
            return nullptr;
        } else {
            auto block = std::make_unique<parse::BlockNode>(m_tokenIndex - 1);
            block->arguments = parseArgDecls();
            block->variables = parseFuncVarDecls();
            block->body = parseFuncBody();
            if (m_token.name != Lexer::Token::Name::kCloseCurly) {
                m_errorReporter->addError(fmt::format("Error parsing function on line {}, expecting closing curly "
                        "brace '}}' to match opening curly brace '{{' on line {}.",
                        m_errorReporter->getLineNumber(m_token.range.data()),
                        m_errorReporter->getLineNumber(openCurly.range.data())));
                return nullptr;
            }
            next(); // }
            return block;
        }
    }

    case Lexer::Token::Name::kMinus:
    case Lexer::Token::Name::kLiteral:
        expr = parseLiteral();
        break;

    default:
        return nullptr;
    }

    if (expr == nullptr) {
        return nullptr;
    }

    bool hasSuffices = true;
    do {
        if (isSingleExpression && m_token.name == Lexer::Token::Name::kOpenSquare) {
            // expr -> expr1: expr1 '[' arglist1 ']' '=' expr
            // expr: expr1 '[' arglist1 ']'
            hasSuffices = false;
        } else {
            if (m_token.name == Lexer::Token::Name::kDot) {
                next(); // .
                if (m_token.name == Lexer::Token::kIdentifier) {
                    next(); // name
                    if (m_token.name == Lexer::Token::kAssign) {
                        // expr: expr '.' name '=' expr
                        auto setter = std::make_unique<parse::SetterNode>(m_tokenIndex - 1);
                        next(); // =
                        setter->target = std::move(expr);
                        setter->value = parseExpr();
                        expr = std::move(setter);
                    } else if (m_token.name == Lexer::Token::kOpenParen) {
                        // expr -> expr1 -> msgsend: expr '.' name '(' keyarglist1 optcomma ')' blocklist
                        // expr -> expr1 -> msgsend: expr '.' name '(' ')' blocklist
                        // expr -> expr1 -> msgsend: expr '.' name '(' arglist1 optkeyarglist ')' blocklist
                        auto call = std::make_unique<parse::CallNode>(m_tokenIndex - 1);
                        Lexer::Token openParen = m_token;
                        next(); // (
                        call->target = std::move(expr);
                        auto args = parseArgList();
                        bool blocklist = true;
                        if (m_token.name == Lexer::Token::kAsterisk) {
                            // TODO: expr -> expr1 -> msgsend: expr '.' name '(' arglistv1 optkeyarglist ')'
                            // Shorthand for performList transformation
                            // target.selector(a, b, *array) => target.performList(\selector, a, b, array)
                            return nullptr;
                        } else {
                            call->arguments = std::move(args);
                        }
                        call->keywordArguments = parseKeywordArgList();
                        if (m_token.name != Lexer::Token::kCloseParen) {
                            m_errorReporter->addError(fmt::format("Error parsing method call on line {}, expecting "
                                "closing parenthesis ')' to match opening parenthesis '(' on line {}.",
                                m_errorReporter->getLineNumber(m_token.range.data()),
                                m_errorReporter->getLineNumber(openParen.range.data())));
                        }
                        next(); // )
                        // TODO: blocklist, if present, gets appended to arglist
                        if (blocklist) {
                            auto blocks = parseBlockList();
                            if (blocks) {
                                if (call->arguments) {
                                    call->arguments->append(std::move(blocks));
                                } else {
                                    call->arguments = std::move(blocks);
                                }
                            }
                        }
                        expr = std::move(call);
                    } else {
                        // expr -> expr1 -> msgsend: expr '.' name blocklist
                        auto call = std::make_unique<parse::CallNode>(m_tokenIndex - 1);
                        call->target = std::move(expr);
                        expr = std::move(call);
                    }
                } else if (m_token.name == Lexer::Token::Name::kOpenSquare) {
                    // expr: expr '.' '[' arglist1 ']'
                    // expr: expr '.' '[' arglist1 ']' '=' expr
                    hasSuffices = false;
                } else if (m_token.name == Lexer::Token::Name::kOpenParen) {
                    // expr -> expr1 -> msgsend: expr '.' '(' ')' blocklist
                    // expr -> expr1 -> msgsend: expr '.' '(' keyarglist1 optcomma ')' blocklist
                    // expr -> expr1 -> msgsend: expr '.' '(' arglist1 optkeyarglist ')' blocklist
                    // expr -> expr1 -> msgsend: expr '.' '(' arglistv1 optkeyarglist ')'
                    hasSuffices = false;
                }
            } else if (m_token.couldBeBinop || m_token.name == Lexer::Token::Name::kKeyword) {
                // expr: expr binop2 adverb expr %prec binop
                // adverb: <e> | '.' name | '.' integer | '.' '(' exprseq ')'
                auto binopCall = std::make_unique<parse::BinopCallNode>(m_tokenIndex);
                next(); // binop2
                // TODO: adverb
                binopCall->rightHand = parseExpr();
                if (!binopCall->rightHand) {
                    return nullptr;
                }
                binopCall->leftHand = std::move(expr);
                expr = std::move(binopCall);
            } else {
                hasSuffices = false;
            }
        }
    } while (hasSuffices);


    return expr;
}

// slotliteral: integer | floatp | ascii | string | symbol | trueobj | falseobj | nilobj | listlit
// integer: INTEGER | '-'INTEGER %prec UMINUS
// floatr: SC_FLOAT | '-' SC_FLOAT %prec UMINUS
std::unique_ptr<parse::LiteralNode> Parser::parseLiteral() {
    std::unique_ptr<parse::LiteralNode> literal;
    if (m_token.name == Lexer::Token::Name::kLiteral) {
        literal = std::make_unique<parse::LiteralNode>(m_tokenIndex, m_token.value);
        next(); // literal
    } else if (m_token.name == Lexer::Token::Name::kMinus && m_tokenIndex < m_lexer.tokens().size() - 1 &&
               m_lexer.tokens()[m_tokenIndex + 1].name == Lexer::Token::Name::kLiteral) {
        if (m_lexer.tokens()[m_tokenIndex + 1].value.type() == Type::kFloat) {
            next(); // '-'
            literal = std::make_unique<parse::LiteralNode>(m_tokenIndex - 1,
                    Literal(-1.0f * m_token.value.asFloat()));
            next(); // literal
        } else if (m_lexer.tokens()[m_tokenIndex + 1].value.type() == Type::kInteger) {
            next(); // '-'
            literal = std::make_unique<parse::LiteralNode>(m_tokenIndex - 1,
                    Literal(-1 * m_token.value.asInteger()));
            next(); // literal
        }
    }
    return literal;
}

// arrayelems: <e> | arrayelems1 optcomma
// arrayelems1: exprseq
//            | exprseq ':' exprseq
//            | keybinop exprseq
//            | arrayelems1 ',' exprseq
//            | arrayelems1 ',' keybinop exprseq
//            | arrayelems1 ',' exprseq ':' exprseq
std::unique_ptr<parse::Node> Parser::parseArrayElements() {
    std::unique_ptr<parse::Node> firstElem;
    if (m_token.name == Lexer::Token::Name::kKeyword) {
        // keybinop exprseq
        firstElem = std::make_unique<parse::LiteralNode>(m_tokenIndex, Literal(Type::kSymbol));
        next(); // keyword
        auto exprSeq = parseExprSeq();
        if (!exprSeq) {
            return nullptr;
        }
        firstElem->append(std::move(exprSeq));
    } else {
        firstElem = parseExprSeq();
        if (!firstElem) {
            return nullptr;
        }
        if (m_token.name == Lexer::Token::Name::kColon) {
            next(); // :
            auto exprSeq = parseExprSeq();
            if (!exprSeq) {
                return nullptr;
            }
            firstElem->append(std::move(exprSeq));
        }
    }

    while (m_token.name == Lexer::Token::Name::kComma) {
        next(); // ,
        if (m_token.name == Lexer::Token::Name::kKeyword) {
            firstElem->append(std::make_unique<parse::LiteralNode>(m_tokenIndex,
                    Literal(Type::kSymbol)));
            next(); // keyword
            auto exprSeq = parseExprSeq();
            if (!exprSeq) {
                return nullptr;
            }
            firstElem->append(std::move(exprSeq));
        } else {
            auto exprSeq = parseExprSeq();
            if (!exprSeq) {
                return firstElem;
            }
            firstElem->append(std::move(exprSeq));
            if (m_token.name == Lexer::Token::Name::kColon) {
                next(); // :
                auto secondSeq = parseExprSeq();
                if (!secondSeq) {
                    return nullptr;
                }
                firstElem->append(std::move(secondSeq));
            }
        }
    }

    return firstElem;
}

// arglist1: exprseq | arglist1 ',' exprseq
std::unique_ptr<parse::Node> Parser::parseArgList() {
    auto seq = parseExprSeq();
    if (seq == nullptr) {
        return nullptr;
    }
    while (m_token.name == Lexer::Token::Name::kComma) {
        next(); // ,
        auto nextSeq = parseExprSeq();
        if (!nextSeq) {
            return seq;
        }
        seq->append(std::move(nextSeq));
    }

    return seq;
}

// keyarglist1: keyarg | keyarglist1 ',' keyarg
// keyarg: keybinop exprseq
std::unique_ptr<parse::KeyValueNode> Parser::parseKeywordArgList() {
    if (m_token.name != Lexer::Token::Name::kKeyword) {
        return nullptr;
    }

    auto keyValue = std::make_unique<parse::KeyValueNode>(m_tokenIndex);
    next(); // keyword
    keyValue->value = parseExprSeq();
    if (keyValue->value == nullptr) {
        return nullptr;
    }
    while (m_token.name == Lexer::Token::Name::kComma) {
        next(); // ,
        if (m_token.name != Lexer::Token::Name::kKeyword) {
            return keyValue;
        }
        auto nextKeyValue = std::make_unique<parse::KeyValueNode>(m_tokenIndex);
        next(); // keyword
        nextKeyValue->value = parseExprSeq();
        if (nextKeyValue->value == nullptr) {
            return nullptr;
        }
        keyValue->append(std::move(nextKeyValue));
    }
    return keyValue;
}

// blocklist1: blocklistitem | blocklist1 blocklistitem
// blocklistitem : blockliteral | generator
// blocklist: <e> | blocklist1
// blockliteral: block
// generator: '{' ':' exprseq ',' qual '}'
//          | '{' ';' exprseq  ',' qual '}'
// block: '{' argdecls funcvardecls funcbody '}'
//      | BEGINCLOSEDFUNC argdecls funcvardecls funcbody '}'
std::unique_ptr<parse::Node> Parser::parseBlockList() {
    if (m_token.name != Lexer::Token::Name::kOpenCurly) {
        return nullptr;
    }
    auto block = parseBlockOrGenerator();
    if (!block) {
        return nullptr;
    }
    while (m_token.name == Lexer::Token::Name::kOpenCurly) {
        auto nextBlock = parseBlockOrGenerator();
        if (!nextBlock) {
            return block;
        }
        block->append(std::move(nextBlock));
    }
    return block;
}

std::unique_ptr<parse::Node> Parser::parseBlockOrGenerator() {
    assert(m_token.name == Lexer::Token::Name::kOpenCurly);

    auto block = std::make_unique<parse::BlockNode>(m_tokenIndex);
    Lexer::Token openBrace = m_token;
    next(); // {
    // TODO: generator
    block->arguments = parseArgDecls();
    block->variables = parseFuncVarDecls();
    block->body = parseFuncBody();
    if (m_token.name != Lexer::Token::Name::kCloseCurly) {
        m_errorReporter->addError(fmt::format("Error parsing block at line {}, expecting closing curly brace '}}' to "
                    "match opening curly brace '}}' at line {}", m_errorReporter->getLineNumber(m_token.range.data()),
                    m_errorReporter->getLineNumber(openBrace.range.data())));
        return nullptr;
    }
    next(); // }
    return block;
}

} // namespace hadron
