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

Parser::~Parser() {}

bool Parser::parse() {
    if (!m_lexer.lex()) {
        return false;
    }

    if (m_lexer.tokens().size() > 0) {
        m_token = m_lexer.tokens()[0];
    } else {
        m_token = Lexer::Token(Lexer::Token::Type::kEmpty, nullptr, 0);
    }

    m_root = parseRoot();
    if (!m_root) return false;

    while (m_errorReporter->errorCount() == 0 && m_token.type != Lexer::Token::Type::kEmpty) {
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
    m_token = Lexer::Token(Lexer::Token::Type::kEmpty, nullptr, 0);
    return false;
}

// Some design conventions around the hand-coded parser:
// Entry conditions are documented with asserts().
// Tail recursion is avoided using the while/node->append pattern.

// root: classes | classextensions | cmdlinecode
// classes: <e> | classes classdef
// classextensions: classextension | classextensions classextension
std::unique_ptr<parse::Node> Parser::parseRoot() {
    switch (m_token.type) {
        case Lexer::Token::Type::kEmpty:
            return std::make_unique<parse::Node>(parse::NodeType::kEmpty, m_tokenIndex);

        case Lexer::Token::Type::kClassName:
            return parseClass();

        case Lexer::Token::Type::kPlus:
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
    assert(m_token.type == Lexer::Token::Type::kClassName);
    auto classNode = std::make_unique<parse::ClassNode>(m_tokenIndex, std::string_view(m_token.start, m_token.length));
    next(); // classname

    if (m_token.type == Lexer::Token::Type::kOpenSquare) {
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

    if (m_token.type == Lexer::Token::Type::kColon) {
        next(); // :
        if (m_token.type != Lexer::Token::Type::kClassName) {
            m_errorReporter->addError(fmt::format("Error parsing class {} at line {}: expecting superclass name after "
                "colon ':'.", classNode->className, m_errorReporter->getLineNumber(m_token.start)));
            return classNode;
        }
        classNode->superClassName = std::string_view(m_token.start, m_token.length);
        next(); // superclass classname
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
        m_errorReporter->addError(fmt::format("Error parsing class {} at line {}: expecting closing curly brace '}}' "
            "to match opening brace '{{' on line {}", classNode->className,
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
        return std::make_unique<parse::ClassExtNode>(m_tokenIndex, std::string_view());
    }
    auto extension = std::make_unique<parse::ClassExtNode>(m_tokenIndex,
            std::string_view(m_token.start, m_token.length));
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
        m_errorReporter->addError(fmt::format("Error parsing around line {}: expecting closing curly brace '}}' to "
                "match opening brace '{{' on line {}", m_errorReporter->getLineNumber(m_token.start),
                m_errorReporter->getLineNumber(openCurly.start)));
    }
    next(); // }
    return extension;
}

// cmdlinecode: '(' funcvardecls1 funcbody ')'
//            | funcvardecls1 funcbody
//            | funcbody
std::unique_ptr<parse::Node> Parser::parseCmdLineCode() {
    switch (m_token.type) {
    case Lexer::Token::Type::kOpenParen: {
        auto openParenToken = m_token;
        next(); // (
        auto block = std::make_unique<parse::BlockNode>(m_tokenIndex);
        block->variables = parseFuncVarDecls();
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
    if (m_token.type == Lexer::Token::Type::kAsterisk) {
        isClassMethod = true;
        next(); // *
    }
    if (m_token.type == Lexer::Token::Type::kIdentifier || m_token.couldBeBinop) {
        auto method = std::make_unique<parse::MethodNode>(m_tokenIndex, std::string_view(m_token.start, m_token.length),
                isClassMethod);
        next(); // name or binop (treated as name)
        if (m_token.type != Lexer::Token::Type::kOpenCurly) {
            m_errorReporter->addError(fmt::format("Error parsing method named '{}' at line {}, expecting opening curly "
                        "brace '{{'.", method->methodName, m_errorReporter->getLineNumber(m_token.start)));
            return nullptr;
        }
        next(); // {
        method->arguments = parseArgDecls();
        method->variables = parseFuncVarDecls();

        if (m_token.type == Lexer::Token::Type::kPrimitive) {
            method->primitive = std::string_view(m_token.start, m_token.length);
            next(); // primitive
            if (m_token.type == Lexer::Token::Type::kSemicolon) {
                next(); // optsemi
            }
        }

        method->body = parseMethodBody();
        if (m_token.type != Lexer::Token::Type::kCloseCurly) {
            m_errorReporter->addError(fmt::format("Error parsing method named '{}' at line {}, expecting closing curly "
                        "brace '}}'.", method->methodName, m_errorReporter->getLineNumber(m_token.start)));
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
//         | exprseq funretval
// funretval: <e> | '^' expr optsemi
std::unique_ptr<parse::Node> Parser::parseFuncBody() {
    if (m_token.type == Lexer::Token::Type::kCaret) {
        auto retNode = std::make_unique<parse::ReturnNode>(m_tokenIndex);
        next(); // ^
        retNode->valueExpr = parseExpr();
        if (m_token.type == Lexer::Token::Type::kSemicolon) {
            next(); // ;
        }
        return retNode;
    }
    auto exprSeq = parseExprSeq();
    if (m_token.type == Lexer::Token::Type::kCaret) {
        auto retNode = std::make_unique<parse::ReturnNode>(m_tokenIndex);
        next(); // ^
        retNode->valueExpr = parseExpr();
        if (m_token.type == Lexer::Token::Type::kSemicolon) {
            next(); // ;
        }
        exprSeq->append(std::move(retNode));
    }
    return exprSeq;
}

// rwslotdeflist: rwslotdef | rwslotdeflist ',' rwslotdef
std::unique_ptr<parse::VarListNode> Parser::parseRWVarDefList() {
    assert(m_token.type == Lexer::Token::Type::kVar || m_token.type == Lexer::Token::Type::kClassVar);
    auto varList = std::make_unique<parse::VarListNode>(m_tokenIndex);
    next(); // var or classvar
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
    bool readAccess = false;
    bool writeAccess = false;

    if (m_token.type == Lexer::Token::Type::kLessThan) {
        readAccess = true;
        next(); // <
    } else if (m_token.type == Lexer::Token::Type::kGreaterThan) {
        writeAccess = true;
        next(); // >
    } else if (m_token.type == Lexer::Token::Type::kReadWriteVar) {
        readAccess = true;
        writeAccess = true;
        next(); // <>
    }

    if (m_token.type != Lexer::Token::Type::kIdentifier) {
        m_errorReporter->addError(fmt::format("Error parsing class variable declaration at line {}, expecting variable "
                    "name.", m_errorReporter->getLineNumber(m_token.start)));
        return nullptr;
    }

    auto varDef = std::make_unique<parse::VarDefNode>(m_tokenIndex, std::string_view(m_token.start, m_token.length));
    next(); // name

    varDef->hasReadAccessor = readAccess;
    varDef->hasWriteAccessor = writeAccess;

    if (m_token.type == Lexer::Token::Type::kAssign) {
        next(); // =
        varDef->initialValue = parseLiteral();
        if (varDef->initialValue == nullptr) {
            m_errorReporter->addError(fmt::format("Error parsing class variable declaration at line {}, expecting "
                    "literal (e.g. number, string, symbol) following assignment.",
                    m_errorReporter->getLineNumber(m_token.start)));
            return nullptr;
        }
    }

    return varDef;
}

// constdeflist: constdef | constdeflist optcomma constdef
// optcomma: <e> | ','
std::unique_ptr<parse::VarListNode> Parser::parseConstDefList() {
    assert(m_token.type == Lexer::Token::Type::kConst);
    auto varList = std::make_unique<parse::VarListNode>(m_tokenIndex);
    next(); // const
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
    bool readAccess = false;
    if (m_token.type == Lexer::Token::Type::kLessThan) {
        readAccess = true;
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
    auto varDef = std::make_unique<parse::VarDefNode>(m_tokenIndex, std::string_view(m_token.start, m_token.length));
    next(); // name
    varDef->hasReadAccessor = readAccess;
    if (m_token.type != Lexer::Token::Type::kAssign) {
        m_errorReporter->addError(fmt::format("Error parsing class constant '{}' declaration at line {}, expecting "
                    "assignment operator '='.", varDef->varName, m_errorReporter->getLineNumber(m_token.start)));
        return nullptr;
    }
    next(); // =
    varDef->initialValue = parseLiteral();
    if (varDef->initialValue == nullptr) {
        m_errorReporter->addError(fmt::format("Error parsing class constant '{}' declaration at line {}, expecting "
                    "literal (e.g. number, string, symbol) following assignment.", varDef->varName,
                    m_errorReporter->getLineNumber(m_token.start)));
        return nullptr;
    }
    return varDef;
}

// vardeflist: vardef | vardeflist ',' vardef
std::unique_ptr<parse::VarListNode> Parser::parseVarDefList() {
    auto varList = std::make_unique<parse::VarListNode>(m_tokenIndex);
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
    auto varDef = std::make_unique<parse::VarDefNode>(m_tokenIndex, std::string_view(m_token.start, m_token.length));
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

// argdecls: <e>
//         | ARG vardeflist ';'
//         | ARG vardeflist0 ELLIPSIS name ';'
//         | '|' slotdeflist '|'
//         | '|' slotdeflist0 ELLIPSIS name '|'
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
    auto argList = std::make_unique<parse::ArgListNode>(m_tokenIndex);
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
        auto retVal = std::make_unique<parse::ReturnNode>(m_tokenIndex);
        next(); // ^
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

    switch (m_token.type) {
    case Lexer::Token::Type::kClassName: {
        Lexer::Token className = m_token;
        next();  // classname
        if (m_token.type == Lexer::Token::kOpenSquare) {
            // expr -> expr1 -> msgsend: classname '[' arrayelems ']'
            auto dynList = std::make_unique<parse::DynListNode>(m_tokenIndex - 1);
            Lexer::Token openSquare = m_token;
            next(); // [
            dynList->className = std::string_view(className.start, className.length);
            dynList->elements = parseArrayElements();
            if (m_token.type != Lexer::Token::kCloseSquare) {
                m_errorReporter->addError(fmt::format("Error parsing dynamic list on line {}, expecting closing square "
                        "bracket ']' to match opening square bracket '[' on line {}",
                        m_errorReporter->getLineNumber(m_token.start),
                        m_errorReporter->getLineNumber(openSquare.start)));
            }
            next(); // ]
            expr = std::move(dynList);
            isSingleExpression = true;
        } else if (m_token.type == Lexer::Token::kOpenParen) {
            // expr -> expr1 -> msgsend: classname '(' ')' blocklist
            // expr -> expr1 -> msgsend: classname '(' keyarglist1 optcomma ')' blocklist
            // expr -> expr1 -> msgsend: classname '(' arglist1 optkeyarglist ')' blocklist
            // expr -> expr1 -> msgsend: classname '(' arglistv1 optkeyarglist ')'
        } else if (m_token.type == Lexer::Token::kOpenCurly) {
            // expr -> expr1 -> msgsend: classname blocklist1
        } else {
            // expr: classname
            expr = std::make_unique<parse::NameNode>(m_tokenIndex - 1, std::string_view(className.start,
                className.length));
        }
    } break;

    case Lexer::Token::Type::kIdentifier: {
        // expr: name '(' arglist1 optkeyarglist ')' '=' expr
        // expr -> expr1 -> msgsend: name blocklist1
        // expr -> expr1 -> msgsend: name '(' ')' blocklist1
        // expr -> expr1 -> msgsend: name '(' arglist1 optkeyarglist ')' blocklist
        // expr -> expr1 -> msgsend: name '(' arglistv1 optkeyarglist ')'
        auto name = std::make_unique<parse::NameNode>(m_tokenIndex, std::string_view(m_token.start, m_token.length));
        next(); // name
        if (m_token.type == Lexer::Token::kAssign) {
            // expr: name '=' expr
            auto assign = std::make_unique<parse::AssignNode>(m_tokenIndex);
            next(); // =
            assign->name = std::move(name);
            assign->value = parseExpr();
            expr = std::move(assign);
        } else {
            // expr -> expr1 -> pushname: name
            expr = std::move(name);
            isSingleExpression = true;
        }
    } break;

    case Lexer::Token::Type::kGrave:
        // expr: '`' expr
        break;

    case Lexer::Token::Type::kTilde: {
        // expr: '~' name '=' expr
        // expr -> expr1: '~' name
        next(); // ~
        if (m_token.type == Lexer::Token::kIdentifier) {
            auto name = std::make_unique<parse::NameNode>(m_tokenIndex, std::string_view(m_token.start,
                    m_token.length));
            name->isGlobal = true;
            next(); // name
            if (m_token.type == Lexer::Token::kAssign) {
                auto assign = std::make_unique<parse::AssignNode>(m_tokenIndex);
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
                    "symbol '~'.", m_errorReporter->getLineNumber(m_token.start)));
            return nullptr;
        }
    } break;

    case Lexer::Token::Type::kHash:
        // expr: '#' mavars '=' expr
        break;

    case Lexer::Token::Type::kOpenParen:
        // expr -> expr1: '(' exprseq ')'
        // expr -> expr1: '(' valrange2 ')'
        // expr -> expr1: '(' ':' valrange3 ')'
        // expr -> expr1: '(' dictslotlist ')'
        // expr -> expr1 -> msgsend: '(' binop2 ')' blocklist1
        // expr -> expr1 -> msgsend: '(' binop2 ')' '(' ')' blocklist1
        // expr -> expr1 -> msgsend: '(' binop2 ')' '(' arglist1 optkeyarglist ')' blocklist
        // expr -> expr1 -> msgsend: '(' binop2 ')' '(' arglistv1 optkeyarglist ')'
        break;

    case Lexer::Token::Type::kOpenSquare: {
        // expr -> expr1: '[' arrayelems ']'
        auto dynList = std::make_unique<parse::DynListNode>(m_tokenIndex);
        Lexer::Token openSquare = m_token;
        next(); // [
        dynList->elements = parseArrayElements();
        if (m_token.type != Lexer::Token::kCloseSquare) {
            m_errorReporter->addError(fmt::format("Error parsing dynamic list on line {}, expecting closing square "
                    "bracket ']' to match opening square bracket '[' on line {}",
                    m_errorReporter->getLineNumber(m_token.start),
                    m_errorReporter->getLineNumber(openSquare.start)));
        }
        next(); // ]
        expr = std::move(dynList);
        isSingleExpression = true;
    } break;

    case Lexer::Token::Type::kOpenCurly: {
        // expr -> expr1 -> generator: '{' ':' exprseq ',' qual '}'
        // expr -> expr1 -> generator: '{' ';' exprseq  ',' qual '}'
        // expr -> expr1 -> blockliteral -> block -> '{' argdecls funcvardecls funcbody '}'
        // expr -> expr1 -> blockliteral -> block -> BEGINCLOSEDFUNC argdecls funcvardecls funcbody '}'
        Lexer::Token openCurly = m_token;
        next(); // {
        if (m_token.type == Lexer::Token::Type::kColon || m_token.type == Lexer::Token::Type::kSemicolon) {
            // Generator expressions parse here.
            return nullptr;
        } else {
            auto block = std::make_unique<parse::BlockNode>(m_tokenIndex - 1);
            block->arguments = parseArgDecls();
            block->variables = parseFuncVarDecls();
            block->body = parseFuncBody();
            if (m_token.type != Lexer::Token::Type::kCloseCurly) {
                m_errorReporter->addError(fmt::format("Error parsing function on line {}, expecting closing curly "
                        "brace '}}' to match opening curly brace '{{' on line {}.",
                        m_errorReporter->getLineNumber(m_token.start),
                        m_errorReporter->getLineNumber(openCurly.start)));
                return nullptr;
            }
            next(); // }
            return block;
        }
    }

    case Lexer::Token::Type::kMinus:
    case Lexer::Token::Type::kLiteral:
        expr = parseLiteral();
        break;

    default:
        return nullptr;
    }

    if (expr == nullptr) {
        return nullptr;
    }

    if (isSingleExpression && m_token.type == Lexer::Token::Type::kOpenSquare) {
        // expr -> expr1: expr1 '[' arglist1 ']' '=' expr
        // expr: expr1 '[' arglist1 ']'
    } else {
        if (m_token.type == Lexer::Token::Type::kDot) {
            next(); // .
            if (m_token.type == Lexer::Token::kIdentifier) {
                Lexer::Token name = m_token;
                next(); // name
                if (m_token.type == Lexer::Token::kAssign) {
                    // expr: expr '.' name '=' expr
                    auto setter = std::make_unique<parse::SetterNode>(m_tokenIndex);
                    next(); // =
                    setter->target = std::move(expr);
                    setter->selector = std::string_view(name.start, name.length);
                    setter->value = parseExpr();
                    expr = std::move(setter);
                }
                // expr -> expr1 -> msgsend: expr '.' name '(' keyarglist1 optcomma ')' blocklist
                // expr -> expr1 -> msgsend: expr '.' name '(' ')' blocklist
                // expr -> expr1 -> msgsend: expr '.' name '(' arglist1 optkeyarglist ')' blocklist
                // expr -> expr1 -> msgsend: expr '.' name '(' arglistv1 optkeyarglist ')'
                // expr -> expr1 -> msgsend: expr '.' name blocklist
            } else if (m_token.type == Lexer::Token::Type::kOpenSquare) {
                // expr: expr '.' '[' arglist1 ']'
                // expr: expr '.' '[' arglist1 ']' '=' expr
            } else if (m_token.type == Lexer::Token::Type::kOpenParen) {
                // expr -> expr1 -> msgsend: expr '.' '(' ')' blocklist
                // expr -> expr1 -> msgsend: expr '.' '(' keyarglist1 optcomma ')' blocklist
                // expr -> expr1 -> msgsend: expr '.' '(' arglist1 optkeyarglist ')' blocklist
                // expr -> expr1 -> msgsend: expr '.' '(' arglistv1 optkeyarglist ')'
            }
        } else if (m_token.couldBeBinop) {
            // expr: expr binop2 adverb expr %prec BINOP
            // adverb: <e> | '.' name | '.' integer | '.' '(' exprseq ')'
        }
    }

    return expr;
}

// slotliteral: integer | floatp | ascii | string | symbol | trueobj | falseobj | nilobj | listlit
// integer: INTEGER | '-'INTEGER %prec UMINUS
// floatr: SC_FLOAT | '-' SC_FLOAT %prec UMINUS
std::unique_ptr<parse::LiteralNode> Parser::parseLiteral() {
    std::unique_ptr<parse::LiteralNode> literal;
    if (m_token.type == Lexer::Token::Type::kLiteral) {
        literal = std::make_unique<parse::LiteralNode>(m_tokenIndex, m_token.value);
        next(); // literal
    } else if (m_token.type == Lexer::Token::Type::kMinus && m_tokenIndex < m_lexer.tokens().size() - 1 &&
               m_lexer.tokens()[m_tokenIndex + 1].type == Lexer::Token::Type::kLiteral) {
        if (m_lexer.tokens()[m_tokenIndex + 1].value.type() == TypedValue::Type::kFloat) {
            next(); // '-'
            literal = std::make_unique<parse::LiteralNode>(m_tokenIndex - 1,
                    TypedValue(-1.0 * m_token.value.asFloat()));
            next(); // literal
        } else if (m_lexer.tokens()[m_tokenIndex + 1].value.type() == TypedValue::Type::kInteger) {
            next(); // '-'
            literal = std::make_unique<parse::LiteralNode>(m_tokenIndex - 1,
                    TypedValue(-1 * m_token.value.asInteger()));
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
    return nullptr;
}

} // namespace hadron
