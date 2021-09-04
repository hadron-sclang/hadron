%%{
    machine lexer;

    action marker { marker = p; }
    action counter { ++counter; }
    action eof_ok { return true; }
    action newline {
            while (lineEndings.size() && lineEndings.back() > p) {
                lineEndings.pop_back();
            }
            lineEndings.emplace_back(p);
        }

    main := |*
        ############
        # comments #
        ############
        '/*' ((extend - ('\n' | '*/')) | ('\n' @newline))* '*/' { /* ignore block comments */ };
        '//' (extend - '\n')* ('\n' @newline >/ eof_ok) {
            // / ignore line comments (and fix Ragel syntax highlighting in vscode with an extra slash)
        };

        ###################
        # number literals #
        ###################
        # Integer base-10
        digit+ {
            int32_t value = strtol(ts, nullptr, 10);
            m_tokens.emplace_back(Token(ts, te - ts, value));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        # Hex integer base-16. Marker points at first digit past 'x'
        ('0x' %marker) xdigit+ {
            int32_t value = strtol(marker, nullptr, 16);
            m_tokens.emplace_back(Token(ts, te - ts, value));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        # Float base-10
        digit+ '.' digit+ {
            double value = strtod(ts, nullptr);
            m_tokens.emplace_back(Token(ts, te - ts, value));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };

        ###################
        # string literals #
        ###################
        # Double-quoted string. Increments counter on escape characters for length computation.
        '"' (('\\' any %counter) | (extend - '"'))* '"' {
            m_tokens.emplace_back(Token(ts + 1, te - ts - 2, Type::kString, counter > 0));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
            counter = 0;
        };

        ###########
        # symbols #
        ###########
        # Single-quoted symbol. Increments counter on escape characters for length computation.
        '\'' (('\\' any %counter) | (extend - '\''))* '\'' {
            m_tokens.emplace_back(Token(ts + 1, te - ts - 2, Type::kSymbol, counter > 0));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
            counter = 0;
        };
        # Slash symbols.
        '\\' [a-zA-Z0-9_]* {
            m_tokens.emplace_back(Token(ts + 1, te - ts - 1, Type::kSymbol, false));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };

        ##############
        # delimiters #
        ##############
        '(' {
            m_tokens.emplace_back(Token(Token::Name::kOpenParen, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
            SPDLOG_INFO("( line: {}, char: {}", lineNumber, ts - lineStart);
        };
        ')' {
            m_tokens.emplace_back(Token(Token::Name::kCloseParen, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '{' {
            m_tokens.emplace_back(Token(Token::Name::kOpenCurly, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '}' {
            m_tokens.emplace_back(Token(Token::Name::kCloseCurly, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '[' {
            m_tokens.emplace_back(Token(Token::Name::kOpenSquare, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        ']' {
            m_tokens.emplace_back(Token(Token::Name::kCloseSquare, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        ',' {
            m_tokens.emplace_back(Token(Token::Name::kComma, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        ';' {
            m_tokens.emplace_back(Token(Token::Name::kSemicolon, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        ':' {
            m_tokens.emplace_back(Token(Token::Name::kColon, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '^' {
            m_tokens.emplace_back(Token(Token::Name::kCaret, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '~' {
            m_tokens.emplace_back(Token(Token::Name::kTilde, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '#' {
            m_tokens.emplace_back(Token(Token::Name::kHash, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '`' {
            m_tokens.emplace_back(Token(Token::Name::kGrave, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '_' {
            m_tokens.emplace_back(Token(Token::Name::kCurryArgument, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };

        #############
        # operators #
        #############
        '+' {
            m_tokens.emplace_back(Token(Token::Name::kPlus, ts, 1, true, kAddHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '-' {
            m_tokens.emplace_back(Token(Token::Name::kMinus, ts, 1, true, kSubtractHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '*' {
            m_tokens.emplace_back(Token(Token::Name::kAsterisk, ts, 1, true, kMultiplyHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '=' {
            m_tokens.emplace_back(Token(Token::Name::kAssign, ts, 1, true, kAssignHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '<' {
            m_tokens.emplace_back(Token(Token::Name::kLessThan, ts, 1, true, kLessThanHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '>' {
            m_tokens.emplace_back(Token(Token::Name::kGreaterThan, ts, 1, true, kGreaterThanHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '|' {
            m_tokens.emplace_back(Token(Token::Name::kPipe, ts, 1, true, kPipeHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '<>' {
            m_tokens.emplace_back(Token(Token::Name::kReadWriteVar, ts, 2, true, kReadWriteHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '<-' {
            m_tokens.emplace_back(Token(Token::Name::kLeftArrow, ts, 2, true, kLeftArrowHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        ('!' | '@' | '%' | '&' | '*' | '-' | '+' | '=' | '|' | '<' | '>' | '?' | '/')+ {
            m_tokens.emplace_back(Token(Token::Name::kBinop, ts, te - ts, true, hash(ts, te - ts)));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        # We don't include the colon at the end of the keyword to simplify parsing.
        lower (alnum | '_')* ':' {
            m_tokens.emplace_back(Token(Token::Name::kKeyword, ts, te - ts - 1, true, hash(ts, te - ts - 1)));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };

        ############
        # keywords #
        ############
        'arg' {
            m_tokens.emplace_back(Token(Token::Name::kArg, ts, 3, false, kArgHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        'classvar' {
            m_tokens.emplace_back(Token(Token::Name::kClassVar, ts, 8, false, kClassVarHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        'const' {
            m_tokens.emplace_back(Token(Token::Name::kConst, ts, 5, false, kConstHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        'false' {
            m_tokens.emplace_back(Token(ts, 5, false, kFalseHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        'nil' {
            m_tokens.emplace_back(Token(ts, 3, Type::kNil, false, kNilHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        'true' {
            m_tokens.emplace_back(Token(ts, 4, true, kTrueHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        'var' {
            m_tokens.emplace_back(Token(Token::Name::kVar, ts, 3, false, kVarHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        'if' {
            m_tokens.emplace_back(Token(Token::Name::kIf, ts, 2, false, kIfHash));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };

        ###############
        # identifiers #
        ###############
        lower (alnum | '_')* {
            m_tokens.emplace_back(Token(Token::Name::kIdentifier, ts, te - ts, false, hash(ts, te - ts)));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };

        ###############
        # class names #
        ###############
        upper (alnum | '_')* {
            m_tokens.emplace_back(Token(Token::Name::kClassName, ts, te - ts, false, hash(ts, te - ts)));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };

        ########
        # dots #
        ########
        '.' {
            m_tokens.emplace_back(Token(Token::Name::kDot, ts, 1));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '..' {
            m_tokens.emplace_back(Token(Token::Name::kDotDot, ts, 2));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        '...' {
            m_tokens.emplace_back(Token(Token::Name::kEllipses, ts, 3));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };
        # Four or more consecutive dots is a lexing error.
        '.' {4, } {
            return false;
        };

        ##############
        # primitives #
        ##############
        '_' (alnum | '_')+ {
            m_tokens.emplace_back(Token(Token::Name::kPrimitive, ts, te - ts, false, hash(ts, te - ts)));
            m_tokens.back().location = Token::Location{lineEndings.size(), lineEndings.size() ? ts - lineEndings.back() : ts - m_code.data()};
        };

        (space - '\n') | ('\n' @newline) { /* ignore whitespace */ };
        any {
            size_t lineNumber = m_errorReporter->getLineNumber(ts);
            m_errorReporter->addError(fmt::format("Lexing error at line {} character {}: unrecognized token '{}'",
                lineNumber, ts - m_errorReporter->getLineStart(lineNumber), std::string(ts, te - ts)));
            return false;
        };
    *|;
}%%

// Generated file from Ragel input file src/Lexer.rl. Please make edits to that file instead of modifying this directly.

#include "hadron/Lexer.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"
#include "hadron/Type.hpp"
#include "Keywords.hpp"

#include "fmt/core.h"
#include "spdlog/spdlog.h"

#include <cstddef>
#include <stdint.h>
#include <stdlib.h>
#include <vector>

namespace {
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-const-variable"
    %% write data;
#   pragma GCC diagnostic pop
}

namespace hadron {

Lexer::Lexer(std::string_view code):
    m_code(code), m_errorReporter(std::make_shared<ErrorReporter>(true)) {
    m_errorReporter->setCode(m_code.data());
}

Lexer::Lexer(std::string_view code, std::shared_ptr<ErrorReporter> errorReporter):
    m_code(code), m_errorReporter(errorReporter) {}

bool Lexer::lex() {
    // Ragel-required state variables.
    const char* p = m_code.data();
    const char* pe = p + m_code.size();
    const char* eof = pe;
    int cs;
    int act;
    const char* ts;
    const char* te;

    // Some parses need a mid-token marker (like hex numbers) so we declare it here.
    const char* marker = nullptr;
    // Some parses need a counter (like string and symbol).
    int counter = 0;
    // Scanner uses backtracking, so we keep a stack of current line endings, and may need to pop them off if we
    // detect a backtrack.
    std::vector<const char*> lineEndings;

    %% write init;

    %% write exec;

    return true;
}

} // namespace hadron

