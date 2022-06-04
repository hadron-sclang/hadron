%%{
    machine lexer;

    action callBlock { fcall blockComment; }
    action returnBlock { fret; }

    action marker { marker = p; }
    action hasEscape { hasEscapeChars = true; }
    action eof_ok { return true; }
    action newline { m_lineEndings.emplace(p - m_code.data()); }

    binopChar = ('!' | '@' | '%' | '&' | '*' | '-' | '+' | '=' | '|' | '<' | '>' | '?' | '/');
    blockComment := (((any - '\n') | ('\n' @newline))* - ('/*' | '*/')) (('/*' @callBlock) | ('*/' @returnBlock));

    main := |*
        ###################
        # number literals #
        ###################
        # Integer base-10
        digit+ {
            int32_t value = strtol(ts, nullptr, 10);
            m_tokens.emplace_back(Token::makeIntegerLiteral(value, std::string_view(ts, te - ts), getLocation(ts)));
        };
        # Hex integer base-16. Marker points at first digit past 'x'
        ('0x' %marker) xdigit+ {
            int32_t value = strtol(marker, nullptr, 16);
            m_tokens.emplace_back(Token::makeIntegerLiteral(value, std::string_view(ts, te - ts), getLocation(ts)));
        };
        # Float base-10
        digit+ '.' digit+ {
            double value = strtod(ts, nullptr);
            m_tokens.emplace_back(Token::makeFloatLiteral(value, std::string_view(ts, te - ts), getLocation(ts)));
        };
        # Float scientific notation
        digit+ ('.' digit+)? 'e' '-'? digit+ {
            double value = strtod(ts, nullptr);
            m_tokens.emplace_back(Token::makeFloatLiteral(value, std::string_view(ts, te - ts), getLocation(ts)));
        };

        ###################
        # string literals #
        ###################
        # Double-quoted string.
        '"' (('\\' any %hasEscape) | (extend - ('"' | '\\')))* '"' {
            m_tokens.emplace_back(Token::makeString(std::string_view(ts + 1, te - ts - 2), getLocation(ts + 1),
                    hasEscapeChars));
            hasEscapeChars = false;
        };

        ###########
        # symbols #
        ###########
        # Single-quoted symbol.
        '\'' (('\\' any %hasEscape) | (extend - ('\'' | '\\')))* '\'' {
            m_tokens.emplace_back(Token::makeSymbol(std::string_view(ts + 1, te - ts - 2), getLocation(ts + 1),
                    hasEscapeChars));
            hasEscapeChars = false;
        };
        # Slash symbols.
        '\\' [a-zA-Z0-9_]* {
            m_tokens.emplace_back(Token::makeSymbol(std::string_view(ts + 1, te - ts - 1), getLocation(ts + 1),
                    false));
        };

        ######################
        # character literals #
        ######################
        '$' (any - '\\') {
            m_tokens.emplace_back(Token::makeCharLiteral(*(ts + 1), std::string_view(ts + 1, 1), getLocation(ts + 1)));
        };
        '$\\t' {
            m_tokens.emplace_back(Token::makeCharLiteral('\t', std::string_view(ts + 1, 2), getLocation(ts + 1)));
        };
        '$\\n' {
            m_tokens.emplace_back(Token::makeCharLiteral('\n', std::string_view(ts + 1, 2), getLocation(ts + 1)));
        };
        '$\\r' {
            m_tokens.emplace_back(Token::makeCharLiteral('\r', std::string_view(ts + 1, 2), getLocation(ts + 1)));
        };
        '$\\' (any - ('t' | 'n' | 'r')) {
            m_tokens.emplace_back(Token::makeCharLiteral(*(ts + 2), std::string_view(ts + 1, 2), getLocation(ts + 1)));
        };

        ##############
        # delimiters #
        ##############
        '(' {
            m_tokens.emplace_back(Token::make(Token::Name::kOpenParen, std::string_view(ts, 1), getLocation(ts)));
        };
        ')' {
            m_tokens.emplace_back(Token::make(Token::Name::kCloseParen, std::string_view(ts, 1), getLocation(ts)));
        };
        '{' {
            m_tokens.emplace_back(Token::make(Token::Name::kOpenCurly, std::string_view(ts, 1), getLocation(ts)));
        };
        '}' {
            m_tokens.emplace_back(Token::make(Token::Name::kCloseCurly, std::string_view(ts, 1), getLocation(ts)));
        };
        '[' {
            m_tokens.emplace_back(Token::make(Token::Name::kOpenSquare, std::string_view(ts, 1), getLocation(ts)));
        };
        ']' {
            m_tokens.emplace_back(Token::make(Token::Name::kCloseSquare, std::string_view(ts, 1), getLocation(ts)));
        };
        ',' {
            m_tokens.emplace_back(Token::make(Token::Name::kComma, std::string_view(ts, 1), getLocation(ts)));
        };
        ';' {
            m_tokens.emplace_back(Token::make(Token::Name::kSemicolon, std::string_view(ts, 1), getLocation(ts)));
        };
        ':' {
            m_tokens.emplace_back(Token::make(Token::Name::kColon, std::string_view(ts, 1), getLocation(ts)));
        };
        '^' {
            m_tokens.emplace_back(Token::make(Token::Name::kCaret, std::string_view(ts, 1), getLocation(ts)));
        };
        '~' {
            m_tokens.emplace_back(Token::make(Token::Name::kTilde, std::string_view(ts, 1), getLocation(ts)));
        };
        '#' {
            m_tokens.emplace_back(Token::make(Token::Name::kHash, std::string_view(ts, 1), getLocation(ts)));
        };
        '`' {
            m_tokens.emplace_back(Token::make(Token::Name::kGrave, std::string_view(ts, 1), getLocation(ts)));
        };
        '_' {
            m_tokens.emplace_back(Token::make(Token::Name::kCurryArgument, std::string_view(ts, 1), getLocation(ts)));
        };

        #############
        # operators #
        #############
        '+' {
            m_tokens.emplace_back(Token::make(Token::Name::kPlus, std::string_view(ts, 1), getLocation(ts), true));
        };
        '-' {
            m_tokens.emplace_back(Token::make(Token::Name::kMinus, std::string_view(ts, 1), getLocation(ts), true));
        };
        '*' {
            m_tokens.emplace_back(Token::make(Token::Name::kAsterisk, std::string_view(ts, 1), getLocation(ts), true));
        };
        '=' {
            m_tokens.emplace_back(Token::make(Token::Name::kAssign, std::string_view(ts, 1), getLocation(ts), true));
        };
        '<' {
            m_tokens.emplace_back(Token::make(Token::Name::kLessThan, std::string_view(ts, 1), getLocation(ts), true));
        };
        '>' {
            m_tokens.emplace_back(Token::make(Token::Name::kGreaterThan, std::string_view(ts, 1), getLocation(ts),
                    true));
        };
        '|' {
            m_tokens.emplace_back(Token::make(Token::Name::kPipe, std::string_view(ts, 1), getLocation(ts), true));
        };
        '<>' {
            m_tokens.emplace_back(Token::make(Token::Name::kReadWriteVar, std::string_view(ts, 2), getLocation(ts),
                    true));
        };
        '<-' {
            m_tokens.emplace_back(Token::make(Token::Name::kLeftArrow, std::string_view(ts, 2), getLocation(ts), true));
        };
        '#{' {
            m_tokens.emplace_back(Token::make(Token::Name::kBeginClosedFunction, std::string_view(ts, 2),
                    getLocation(ts), false));
        };
        (binopChar+ - (('/*' binopChar*) | ('//' binopChar*))) {
            m_tokens.emplace_back(Token::make(Token::Name::kBinop, std::string_view(ts, te - ts), getLocation(ts),
                    true));
        };
        # We don't include the colon at the end of the keyword to simplify parsing.
        lower (alnum | '_')* ':' {
            m_tokens.emplace_back(Token::make(Token::Name::kKeyword, std::string_view(ts, te - ts - 1), getLocation(ts),
                    true));
        };

        ############
        # keywords #
        ############
        'arg' {
            m_tokens.emplace_back(Token::make(Token::Name::kArg, std::string_view(ts, 3), getLocation(ts), false));
        };
        'classvar' {
            m_tokens.emplace_back(Token::make(Token::Name::kClassVar, std::string_view(ts, 8), getLocation(ts), false));
        };
        'const' {
            m_tokens.emplace_back(Token::make(Token::Name::kConst, std::string_view(ts, 5), getLocation(ts), false));
        };
        'false' {
            m_tokens.emplace_back(Token::makeBooleanLiteral(false, std::string_view(ts, 5), getLocation(ts)));
        };
        'nil' {
            m_tokens.emplace_back(Token::makeNilLiteral(std::string_view(ts, 3), getLocation(ts)));
        };
        'true' {
            m_tokens.emplace_back(Token::makeBooleanLiteral(true, std::string_view(ts, 4), getLocation(ts)));
        };
        'var' {
            m_tokens.emplace_back(Token::make(Token::Name::kVar, std::string_view(ts, 3), getLocation(ts), false));
        };
        'if' {
            m_tokens.emplace_back(Token::make(Token::Name::kIf, std::string_view(ts, 2), getLocation(ts), false));
        };
        'while' {
            m_tokens.emplace_back(Token::make(Token::Name::kWhile, std::string_view(ts, 5), getLocation(ts), false));
        };
        'inf' {
            auto value = std::numeric_limits<double>::infinity();
            m_tokens.emplace_back(Token::makeFloatLiteral(value, std::string_view(ts, 3), getLocation(ts)));
        };
        'pi' {
            m_tokens.emplace_back(Token::makePi(std::string_view(ts, 2), getLocation(ts)));
        };

        ###############
        # identifiers #
        ###############
        lower (alnum | '_')* {
            m_tokens.emplace_back(Token::make(Token::Name::kIdentifier, std::string_view(ts, te - ts), getLocation(ts),
                    false));
        };

        ###############
        # class names #
        ###############
        upper (alnum | '_')* {
            m_tokens.emplace_back(Token::make(Token::Name::kClassName, std::string_view(ts, te - ts), getLocation(ts),
                    false));
        };

        ########
        # dots #
        ########
        '.' {
            m_tokens.emplace_back(Token::make(Token::Name::kDot, std::string_view(ts, 1), getLocation(ts)));
        };
        '..' {
            m_tokens.emplace_back(Token::make(Token::Name::kDotDot, std::string_view(ts, 2), getLocation(ts)));
        };
        '...' {
            m_tokens.emplace_back(Token::make(Token::Name::kEllipses, std::string_view(ts, 3), getLocation(ts)));
        };
        # Four or more consecutive dots is a lexing error.
        '.' {4, } {
            return false;
        };

        ##############
        # primitives #
        ##############
        '_' (alnum | '_')+ {
            m_tokens.emplace_back(Token::make(Token::Name::kPrimitive, std::string_view(ts, te - ts), getLocation(ts),
                    false));
        };

        ############
        # comments #
        ############
        '/*' { fcall blockComment; };

        '//' (any - '\n')* ('\n' @newline >/ eof_ok) {
            // / ignore line comments (and fix Ragel syntax highlighting in vscode with an extra slash)
        };

        ##############
        # whitespace #
        ##############
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
#include "hadron/Slot.hpp"

#include "fmt/core.h"
#include "spdlog/spdlog.h"

#include <array>
#include <cstddef>
#include <limits>
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
    m_code(code), m_errorReporter(errorReporter) {
        m_errorReporter->setCode(m_code.data());
    }

bool Lexer::lex() {
    // Ragel-required state variables.
    const char* p = m_code.data();
    const char* pe = p + m_code.size();
    const char* eof = pe;
    int cs;
    int act;
    const char* ts;
    const char* te;
    // TODO: could consider a dynamically resizing stack, but this is only used for nested block comments right now.
    std::array<int, 32> stack;
    int top = 0;

    // Some parses need a mid-token marker (like hex numbers) so we declare it here.
    const char* marker = nullptr;
    // A flag for string and symbol parsing for if they have escape symbols (so can't be blitted).
    bool hasEscapeChars = false;

    %% write init;

    %% write exec;

    return true;
}

Token::Location Lexer::getLocation(const char* p) {
    size_t lineNumber = m_lineEndings.size();
    size_t offset = p - m_code.data();
    // While the scanner does backtrack, tokens are built from the start to the end of the file, so start looking
    // from the back. Find first element less than offset.
    for (auto iter = m_lineEndings.rbegin(); iter != m_lineEndings.rend(); ++iter) {
        if (*iter < offset) {
            return Token::Location{lineNumber, offset - *iter - 1};
        }
        --lineNumber;
    }

    // Must be on the first line.
    return Token::Location{0, offset};
}

} // namespace hadron

