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
            int64_t value = strtoll(ts, nullptr, 10);
            m_tokens.emplace_back(Token(ts, te - ts, value));
        };
        # Hex integer base-16. Marker points at first digit past 'x'
        ('0x' %marker) xdigit+ {
            int64_t value = strtoll(marker, nullptr, 16);
            m_tokens.emplace_back(Token(ts, te - ts, value));
        };
        # Float base-10
        digit+ '.' digit+ {
            double value = strtod(ts, nullptr);
            m_tokens.emplace_back(Token(ts, te - ts, value));
        };

        ###################
        # string literals #
        ###################
        # Double-quoted string. Increments counter on escape characters for length computation.
        '"' (('\\' any %counter) | (extend - '"'))* '"' {
            m_tokens.emplace_back(Token(ts + 1, te - ts - 2 - counter, Type::kString));
            counter = 0;
        };

        ###########
        # symbols #
        ###########
        # Single-quoted symbol. Increments counter on escape characters for length computation.
        '\'' (('\\' any %counter) | (extend - '\''))* '\'' {
            m_tokens.emplace_back(Token(ts + 1, te - ts - 2 - counter, Type::kSymbol));
            counter = 0;
        };
        # Slash symbols.
        '\\' [a-zA-Z0-9_]* {
            m_tokens.emplace_back(Token(ts + 1, te - ts - 1, Type::kSymbol));
        };

        ##############
        # delimiters #
        ##############
        '(' {
            m_tokens.emplace_back(Token(Token::Name::kOpenParen, ts, 1));
        };
        ')' {
            m_tokens.emplace_back(Token(Token::Name::kCloseParen, ts, 1));
        };
        '{' {
            m_tokens.emplace_back(Token(Token::Name::kOpenCurly, ts, 1));
        };
        '}' {
            m_tokens.emplace_back(Token(Token::Name::kCloseCurly, ts, 1));
        };
        '[' {
            m_tokens.emplace_back(Token(Token::Name::kOpenSquare, ts, 1));
        };
        ']' {
            m_tokens.emplace_back(Token(Token::Name::kCloseSquare, ts, 1));
        };
        ',' {
            m_tokens.emplace_back(Token(Token::Name::kComma, ts, 1));
        };
        ';' {
            m_tokens.emplace_back(Token(Token::Name::kSemicolon, ts, 1));
        };
        ':' {
            m_tokens.emplace_back(Token(Token::Name::kColon, ts, 1));
        };
        '^' {
            m_tokens.emplace_back(Token(Token::Name::kCaret, ts, 1));
        };
        '~' {
            m_tokens.emplace_back(Token(Token::Name::kTilde, ts, 1));
        };
        '#' {
            m_tokens.emplace_back(Token(Token::Name::kHash, ts, 1));
        };
        '`' {
            m_tokens.emplace_back(Token(Token::Name::kGrave, ts, 1));
        };
        '_' {
            m_tokens.emplace_back(Token(Token::Name::kCurryArgument, ts, 1));
        };

        #############
        # operators #
        #############
        '+' {
            m_tokens.emplace_back(Token(Token::Name::kPlus, ts, 1, true));
        };
        '-' {
            m_tokens.emplace_back(Token(Token::Name::kMinus, ts, 1, true));
        };
        '*' {
            m_tokens.emplace_back(Token(Token::Name::kAsterisk, ts, 1, true));
        };
        '=' {
            m_tokens.emplace_back(Token(Token::Name::kAssign, ts, 1, true));
        };
        '<' {
            m_tokens.emplace_back(Token(Token::Name::kLessThan, ts, 1, true));
        };
        '>' {
            m_tokens.emplace_back(Token(Token::Name::kGreaterThan, ts, 1, true));
        };
        '|' {
            m_tokens.emplace_back(Token(Token::Name::kPipe, ts, 1, true));
        };
        '<>' {
            m_tokens.emplace_back(Token(Token::Name::kReadWriteVar, ts, 2, true));
        };
        '<-' {
            m_tokens.emplace_back(Token(Token::Name::kLeftArrow, ts, 2, true));
        };
        ('!' | '@' | '%' | '&' | '*' | '-' | '+' | '=' | '|' | '<' | '>' | '?' | '/')+ {
            m_tokens.emplace_back(Token(Token::Name::kBinop, ts, te - ts, true));
        };
        # We don't include the colon at the end of the keyword to simplify parsing.
        lower (alnum | '_')* ':' {
            m_tokens.emplace_back(Token(Token::Name::kKeyword, ts, te - ts - 1, true));
        };

        ############
        # keywords #
        ############
        'arg' {
            m_tokens.emplace_back(Token(Token::Name::kArg, ts, 3));
        };
        'classvar' {
            m_tokens.emplace_back(Token(Token::Name::kClassVar, ts, 8));
        };
        'const' {
            m_tokens.emplace_back(Token(Token::Name::kConst, ts, 5));
        };
        'false' {
            m_tokens.emplace_back(Token(ts, 5, false));
        };
        'nil' {
            m_tokens.emplace_back(Token(ts, 3, Type::kNil));
        };
        'true' {
            m_tokens.emplace_back(Token(ts, 4, true));
        };
        'var' {
            m_tokens.emplace_back(Token(Token::Name::kVar, ts, 3));
        };

        ###############
        # identifiers #
        ###############
        lower (alnum | '_')* {
            m_tokens.emplace_back(Token(Token::Name::kIdentifier, ts, te - ts));
        };

        ###############
        # class names #
        ###############
        upper (alnum | '_')* {
            m_tokens.emplace_back(Token(Token::Name::kClassName, ts, te - ts));
        };

        ########
        # dots #
        ########
        '.' {
            m_tokens.emplace_back(Token(Token::Name::kDot, ts, 1));
        };
        '..' {
            m_tokens.emplace_back(Token(Token::Name::kDotDot, ts, 2));
        };
        '...' {
            m_tokens.emplace_back(Token(Token::Name::kEllipses, ts, 3));
        };
        # Four or more consecutive dots is a lexing error.
        '.' {4, } {
            return false;
        };

        ##############
        # primitives #
        ##############
        '_' (alnum | '_')+ {
            m_tokens.emplace_back(Token(Token::Name::kPrimitive, ts, te - ts));
        };

        space { /* ignore whitespace */ };
        any {
            if (errorReporter) {
                errorReporter->addError(fmt::format("Lexing error at line {}: unrecognized token '{}'",
                    errorReporter->getLineNumber(ts), std::string(ts, te - ts)));
            }
            return false;
        };
    *|;
}%%

// Generated file from Ragel input file src/Lexer.rl. Please make edits to that file instead of modifying this directly.

#include "Lexer.hpp"
#include "ErrorReporter.hpp"

#include "fmt/core.h"

namespace {
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-const-variable"
    %% write data;
#   pragma GCC diagnostic pop
}

namespace hadron {

Lexer::Lexer(std::string_view code):
    m_code(code) { }

bool Lexer::lex(ErrorReporter* errorReporter) {
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

    %% write init;

    %% write exec;

    return true;
}

} // namespace hadron

