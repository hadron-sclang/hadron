%%{
    machine lexer;

    name = '~'? lower [a-zA-Z0-9_]+;
    const = 'const';
    var = 'var';
    nil = 'nil';
    true = 'true';
    false = 'false';

    radixInteger = '-'? digit+ 'r' alnum+;
    radixFloat = '-'? digit+' r' alnum+ '.' alnum+;

    action marker { marker = p; }
    action flag { flag = true; }
    action counter { ++counter; }

    main := |*

        ###################
        # number literals #
        ###################
        # Integer base-10
        digit+ {
            int64_t value = std::strtoll(ts, nullptr, 10);
            m_tokens.emplace_back(Token(ts, te - ts, value));
        };
        # Hex integer base-16. Marker points at first digit past 'x'
        digit+ ('x' %marker) xdigit+ {
            int64_t value = std::strtoll(marker, nullptr, 16);
            if (flag) { value = -value; flag = false; }
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
        '"' (('\\' any %counter)|(any - '"'))* '"' {
            m_tokens.emplace_back(Token(Token::Type::kString, ts + 1, te - ts - 2 - counter));
            counter = 0;
        };

        ###########
        # symbols #
        ###########
        # Single-quoted symbol. Increments counter on escape characters for length computation.
        '\'' (('\\' any %counter)|(any - '\''))* '\'' {
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
        ',' {
            m_tokens.emplace_back(Token(Token::Type::kComma, ts, 1));
        };

        #############
        # operators #
        #############
        '+' {
            m_tokens.emplace_back(Token(Token::Type::kPlus, ts, 1));
        };
        '-' {
            m_tokens.emplace_back(Token(Token::Type::kMinus, ts, 1));
        };
        '*' {
            m_tokens.emplace_back(Token(Token::Type::kAsterisk, ts, 1));
        };
        '=' {
            m_tokens.emplace_back(Token(Token::Type::kAssign, ts, 1));
        };
        '<' {
            m_tokens.emplace_back(Token(Token::Type::kLessThan, ts, 1));
        };
        '>' {
            m_tokens.emplace_back(Token(Token::Type::kGreaterThan, ts, 1));
        };
        '|' {
            m_tokens.emplace_back(Token(Token::Type::kPipe, ts, 1));
        };
        '<>' {
            m_tokens.emplace_back(Token(Token::Type::kReadWriteVar, ts, 2));
        };
        '<-' {
            m_tokens.emplace_back(Token(Token::Type::kLeftArrow, ts, 2));
        };
        ('!' | '@' | '%' | '&' | '*' | '-' | '+' | '=' | '|' | '<' | '>' | '?' | '/')+ {
            m_tokens.emplace_back(Token(Token::Type::kBinop, ts, te - ts));
        };

        space { /* ignore whitespace */ };
        any { return false; };
    *|;
}%%

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
    // Some parses need a flag (like negative hex numbers) so we declare it here.
    bool flag = false;
    // Some parses need a counter (like string).
    int counter = 0;

    %% write exec;

    return true;
}

} // namespace hadron

