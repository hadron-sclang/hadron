#ifndef SRC_INCLUDE_HADRON_TOKEN_HPP_
#define SRC_INCLUDE_HADRON_TOKEN_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"

#include <string_view>

namespace hadron {

// Lexer lexes source to produce Tokens, Parser consumes Tokens to produce Parse Tree.
struct Token {
    enum Name {
        kEmpty,      // represents no token
        kInterpret,  // The LSC grammar is ambiguous as written without the insertion of a special token informing the
                     // parser that the input text is interpreted code. Without this the grammar cannot determine if
                     // a classname input is a class definition or a reference to a class as part of an expression.
                     // To fix we inject this token at the beginning of interpreted code. There may be other ways to
                     // resolve this ambiguity but they will likely require some changes to the grammar.
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
    Slot value;
    bool couldBeBinop;
    Hash hash = 0;
    bool escapeString = false;

    Token(): name(kEmpty), couldBeBinop(false) {}

    /*! Makes an integer kLiteral token */
    Token(const char* start, size_t length, int32_t intValue):
        name(kLiteral), range(start, length), value(intValue), couldBeBinop(false) {}

    /*! Makes a float kLiteral token */
    Token(const char* start, size_t length, double floatValue):
        name(kLiteral), range(start, length), value(floatValue), couldBeBinop(false) {}

    /*! Makes a boolean kLiteral token */
    Token(const char* start, size_t length, bool boolean, Hash h = 0):
        name(kLiteral), range(start, length), value(boolean), couldBeBinop(false), hash(h) {}

    /*! Makes an kLiteral token */
    Token(const char* start, size_t length, Type literalType, bool hasEscapeCharacters = false, Hash h = 0):
            name(kLiteral),
            range(start, length),
            value(literalType),
            couldBeBinop(false),
            hash(h),
            escapeString(hasEscapeCharacters) {}

    /*! Makes a token with no value storage */
    Token(Name n, const char* start, size_t length, bool binop = false, Hash h = 0):
        name(n), range(start, length), couldBeBinop(binop), hash(h) {}
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_TOKEN_HPP_