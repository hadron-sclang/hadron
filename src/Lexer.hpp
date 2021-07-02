#ifndef SRC_LEXER_HPP_
#define SRC_LEXER_HPP_

#include "Hash.hpp"
#include "Literal.hpp"
#include "Type.hpp"

#include <cstddef>
#include <memory>
#include <stdint.h>
#include <stdlib.h>
#include <string_view>
#include <vector>

namespace hadron {

class ErrorReporter;

class Lexer {
public:
    struct Token {
        enum Name {
            kEmpty,  // represents no token
            kLiteral,
            kPrimitive,

            // <<< all below could also be binops >>>
            kPlus,         // so named because it could be an addition or a class extension
            kMinus,        // Could be unary negation so handled separately
            kAsterisk,     // so named because it could be a multiply or a class method
            kAssign,
            kLessThan,
            kGreaterThan,
            kPipe,
            kReadWriteVar,
            kLeftArrow,
            kBinop,  // TODO: rename kGenericBinop, this is some arbitrary collection of the valid binop characters.
            kKeyword,      // Any identifier with a colon after it.
            // <<< all above could also be binops >>>

            kOpenParen,
            kCloseParen,
            kOpenCurly,
            kCloseCurly,
            kOpenSquare,
            kCloseSquare,
            kComma,
            kSemicolon,
            kColon,
            kCaret,
            kTilde,
            kHash,
            kGrave,
            kVar,
            kArg,
            kConst,
            kClassVar,
            kIdentifier,
            kClassName,
            kDot,
            kDotDot,
            kEllipses,
            kCurryArgument
        };

        Name name;
        std::string_view range;
        Literal value;
        bool couldBeBinop;
        Hash hash;

        Token(): name(kEmpty), couldBeBinop(false) {}

        /*! Makes an integer kLiteral token */
        Token(const char* start, size_t length, int32_t intValue):
            name(kLiteral), range(start, length), value(intValue), couldBeBinop(false) {}

        /*! Makes a float kLiteral token */
        Token(const char* start, size_t length, float doubleValue):
            name(kLiteral), range(start, length), value(doubleValue), couldBeBinop(false) {}

        /*! Makes a boolean kLiteral token */
        Token(const char* start, size_t length, bool boolean):
            name(kLiteral), range(start, length), value(boolean), couldBeBinop(false) {}

        /*! Makes a symbol or string kLiteral token */
        Token(const char* start, size_t length, Type literalType, bool hasEscapeCharacters = false):
            name(kLiteral), range(start, length), value(literalType, hasEscapeCharacters), couldBeBinop(false) {}

        /*! Makes a token with no value storage */
        Token(Name n, const char* start, size_t length, bool binop = false, Hash h = 0):
            name(n), range(start, length), couldBeBinop(binop), hash(h) {}
    };

    Lexer(std::string_view code);
    ~Lexer() = default;

    // For testing, use a local ErrorReporter
    bool lex();
    bool lex(ErrorReporter* errorReporter);

    const std::vector<Token>& tokens() const { return m_tokens; }

    // Access for testing
    const ErrorReporter* errorReporter() const { return m_errorReporter.get(); }

private:
    std::string_view m_code;
    std::vector<Token> m_tokens;

    std::unique_ptr<ErrorReporter> m_errorReporter;
};

} // namespace hadron

#endif // SRC_LEXER_HPP_
