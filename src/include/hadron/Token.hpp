#ifndef SRC_INCLUDE_HADRON_TOKEN_HPP_
#define SRC_INCLUDE_HADRON_TOKEN_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Literal.hpp"

#include <string_view>

namespace hadron {

// Lexer lexes source to produce Tokens, Parser consumes Tokens to produce Parse Tree.
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
    Token(const char* start, size_t length, bool boolean, Hash h = 0):
        name(kLiteral), range(start, length), value(boolean), couldBeBinop(false), hash(h) {}

    /*! Makes an kLiteral token */
    Token(const char* start, size_t length, Type literalType, bool hasEscapeCharacters = false, Hash h = 0):
        name(kLiteral), range(start, length), value(literalType, hasEscapeCharacters), couldBeBinop(false),
        hash(h) {}

    /*! Makes a token with no value storage */
    Token(Name n, const char* start, size_t length, bool binop = false, Hash h = 0):
        name(n), range(start, length), couldBeBinop(binop), hash(h) {}
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_TOKEN_HPP_