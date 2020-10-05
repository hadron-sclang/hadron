%%{
    machine lexer;

    action marker { marker = p; }
    action counter { ++counter; }
    action eof_ok { return true; }

    main := |*
        ############
        # comments #
        ############
        '/*' extend* '*/' { /* ignore block comments */ };
        '//' (extend - '\n')* ('\n' >/eof_ok) { /* ignore line comments */ };

        ###################
        # number literals #
        ###################
        # Integer base-10
        digit+ {
            int64_t value = std::strtoll(ts, nullptr, 10);
            m_tokens.emplace_back(Token(ts, te - ts, value));
        };
        # Hex integer base-16. Marker points at first digit past 'x'
        ('0x' %marker) xdigit+ {
            int64_t value = std::strtoll(marker, nullptr, 16);
            m_tokens.emplace_back(Token(ts, te - ts, value));
        };
        # Float base-10
        digit+ '.' digit+ {
            double value = std::strtod(ts, nullptr);
            m_tokens.emplace_back(Token(ts, te - ts, value));
        };

        ###################
        # string literals #
        ###################
        # Double-quoted string. Increments counter on escape characters for length computation.
        '"' (('\\' any %counter) | (extend - '"'))* '"' {
            m_tokens.emplace_back(Token(Token::Type::kString, ts + 1, te - ts - 2 - counter));
            counter = 0;
        };

        ###########
        # symbols #
        ###########
        # Single-quoted symbol. Increments counter on escape characters for length computation.
        '\'' (('\\' any %counter) | (extend - '\''))* '\'' {
            m_tokens.emplace_back(Token(Token::Type::kSymbol, ts + 1, te - ts - 2 - counter));
            counter = 0;
        };
        # Slash symbols.
        '\\' [a-zA-Z0-9_]* {
            m_tokens.emplace_back(Token(Token::Type::kSymbol, ts + 1, te - ts - 1));
        };

        ##############
        # delimiters #
        ##############
        '(' {
            m_tokens.emplace_back(Token(Token::Type::kOpenParen, ts, 1));
        };
        ')' {
            m_tokens.emplace_back(Token(Token::Type::kCloseParen, ts, 1));
        };
        '{' {
            m_tokens.emplace_back(Token(Token::Type::kOpenCurly, ts, 1));
        };
        '}' {
            m_tokens.emplace_back(Token(Token::Type::kCloseCurly, ts, 1));
        };
        '[' {
            m_tokens.emplace_back(Token(Token::Type::kOpenSquare, ts, 1));
        };
        ']' {
            m_tokens.emplace_back(Token(Token::Type::kCloseSquare, ts, 1));
        };
        ',' {
            m_tokens.emplace_back(Token(Token::Type::kComma, ts, 1));
        };
        ';' {
            m_tokens.emplace_back(Token(Token::Type::kSemicolon, ts, 1));
        };
        ':' {
            m_tokens.emplace_back(Token(Token::Type::kColon, ts, 1));
        };
        '^' {
            m_tokens.emplace_back(Token(Token::Type::kCaret, ts, 1));
        };
        '~' {
            m_tokens.emplace_back(Token(Token::Type::kTilde, ts, 1));
        };
        '#' {
            m_tokens.emplace_back(Token(Token::Type::kHash, ts, 1));
        };

        #############
        # operators #
        #############
        '+' {
            m_tokens.emplace_back(Token(Token::Type::kPlus, ts, 1, true));
        };
        '-' {
            m_tokens.emplace_back(Token(Token::Type::kMinus, ts, 1, true));
        };
        '*' {
            m_tokens.emplace_back(Token(Token::Type::kAsterisk, ts, 1, true));
        };
        '=' {
            m_tokens.emplace_back(Token(Token::Type::kAssign, ts, 1, true));
        };
        '<' {
            m_tokens.emplace_back(Token(Token::Type::kLessThan, ts, 1, true));
        };
        '>' {
            m_tokens.emplace_back(Token(Token::Type::kGreaterThan, ts, 1, true));
        };
        '|' {
            m_tokens.emplace_back(Token(Token::Type::kPipe, ts, 1, true));
        };
        '<>' {
            m_tokens.emplace_back(Token(Token::Type::kReadWriteVar, ts, 2, true));
        };
        '<-' {
            m_tokens.emplace_back(Token(Token::Type::kLeftArrow, ts, 2, true));
        };
        ('!' | '@' | '%' | '&' | '*' | '-' | '+' | '=' | '|' | '<' | '>' | '?' | '/')+ {
            m_tokens.emplace_back(Token(Token::Type::kBinop, ts, te - ts, true));
        };

        ############
        # keywords #
        ############
        'arg' {
            m_tokens.emplace_back(Token(Token::Type::kArg, ts, 3));
        };
        'classvar' {
            m_tokens.emplace_back(Token(Token::Type::kClassVar, ts, 8));
        };
        'const' {
            m_tokens.emplace_back(Token(Token::Type::kConst, ts, 5));
        };
        'false' {
            m_tokens.emplace_back(Token(Token::Type::kFalse, ts, 5));
        };
        'nil' {
            m_tokens.emplace_back(Token(Token::Type::kNil, ts, 3));
        };
        'true' {
            m_tokens.emplace_back(Token(Token::Type::kTrue, ts, 4));
        };
        'var' {
            m_tokens.emplace_back(Token(Token::Type::kVar, ts, 3));
        };

        ###############
        # identifiers #
        ###############
        lower (alnum | '_')* {
            m_tokens.emplace_back(Token(Token::Type::kIdentifier, ts, te - ts));
        };

        ###############
        # class names #
        ###############
        upper (alnum | '_')* {
            m_tokens.emplace_back(Token(Token::Type::kClassName, ts, te - ts));
        };

        ########
        # dots #
        ########
        '.' {
            m_tokens.emplace_back(Token(Token::Type::kDot, ts, 1));
        };
        '..' {
            m_tokens.emplace_back(Token(Token::Type::kDotDot, ts, 2));
        };
        '...' {
            m_tokens.emplace_back(Token(Token::Type::kEllipses, ts, 3));
        };
        # Four or more consecutive dots is a lexing error.
        '.' {4, } {
            return false;
        };

        space { /* ignore whitespace */ };
        any { return false; };
    *|;
}%%

// Generated file from Ragel input file src/Lexer.rl. Please make edits to that file instead of modifying this directly.

#include "Lexer.hpp"

#include "spdlog/spdlog.h"

namespace {
    %% write data;
}

namespace hadron {

Lexer::Lexer(std::string_view code):
    p(code.data()),
    pe(code.data() + code.size()),
    eof(pe) {
    %% write init;
}

bool Lexer::lex() {
    // Some parses need a mid-token marker (like hex numbers) so we declare it here.
    const char* marker = nullptr;
    // Some parses need a counter (like string and symbol).
    int counter = 0;

    %% write exec;

    return true;
}

} // namespace hadron

