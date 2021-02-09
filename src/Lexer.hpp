#ifndef SRC_LEXER_HPP_
#define SRC_LEXER_HPP_

#include "TypedLiteral.hpp"

#include <cstddef>
#include <stdint.h>
#include <stdlib.h>
#include <string_view>
#include <vector>

namespace hadron {

class Lexer {
public:
    struct Token {
        enum Type {
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
        };

        Type type;

        // TODO: refactor to use std::string_view instead of start, length
        /*! Start position of the token. */
        const char* start;

        /*! Length of the token in bytes. */
        size_t length;

        TypedLiteral value;

        bool couldBeBinop;

        Token(): type(kEmpty), start(nullptr), length(0), couldBeBinop(false) {}

        /*! Makes an integer kLiteral token */
        Token(const char* s, size_t l, int64_t intValue):
            type(kLiteral), start(s), length(l), value(intValue), couldBeBinop(false) {}

        /*! Makes a float kLiteral token */
        Token(const char* s, size_t l, double doubleValue):
            type(kLiteral), start(s), length(l), value(doubleValue), couldBeBinop(false) {}

        /*! Makes a boolean kLiteral token */
        Token(const char* s, size_t l, bool boolean):
            type(kLiteral), start(s), length(l), value(boolean), couldBeBinop(false) {}

        /*! Makes a kLiteral with a provided literal type (until we figure out strings and such better) */
        Token(const char*s, size_t l, TypedLiteral::Type t):
            type(kLiteral), start(s), length(l), value(t), couldBeBinop(false) {}

        /*! Makes a token with no value storage */
        Token(Type t, const char* s, size_t l): type(t), start(s), length(l), couldBeBinop(false) {}

        /* Makes a token with possible true value for couldBeBinop */
        Token(Type t, const char* s, size_t l, bool binop): type(t), start(s), length(l), couldBeBinop(binop) {}
    };

    Lexer(std::string_view code);
    bool lex();

    const std::vector<Token>& tokens() const { return m_tokens; }

private:
    std::vector<Token> m_tokens;

    // TODO: these can likely be moved back to lex() as local variables there.
    // Ragel-required state variables.
    const char* p;
    const char* pe;
    const char* eof;
    int cs;
    int act;
    const char* ts;
    const char* te;
};

} // namespace hadron

#endif // SRC_LEXER_HPP_
