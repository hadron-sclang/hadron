#ifndef SRC_COMPILER_INCLUDE_HADRON_TOKEN_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_TOKEN_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"

#include <string_view>

namespace hadron {

// Lexer lexes source to produce Tokens, Parser consumes Tokens to produce Parse Tree.
struct Token {
    enum Name {
        // ** Modifying these? Please make sure to update server/JSONTransport.cpp too!
        kEmpty = 0,     // represents no token
        kInterpret = 1, // The LSC grammar is ambiguous as written without the insertion of a special token informing
                        // the parser that the input text is interpreted code. Without this the grammar cannot determine
                        // if aclassname input is a class definition or a reference to a class as part of an
                        // expression. To fix we inject this token at the beginning of interpreted code. There may be
                        // other ways to resolve this ambiguity but they will likely require some changes to the
                        // grammar.
        kLiteral = 2,
        kPrimitive = 3,

        // <<< all below could also be binops >>>
        kPlus = 4,         // so named because it could be an addition or a class extension
        kMinus = 5,        // Could be unary negation so handled separately
        kAsterisk = 6,     // so named because it could be a multiply or a class method
        kAssign = 7,
        kLessThan = 8,
        kGreaterThan = 9,
        kPipe = 10,
        kReadWriteVar = 11,
        kLeftArrow = 12,
        kBinop = 13,  // TODO: rename kGenericBinop, this is some arbitrary collection of the valid binop characters.
        kKeyword = 14,      // Any identifier with a colon after it.
        // <<< all above could also be binops >>>

        kOpenParen = 15,
        kCloseParen = 16,
        kOpenCurly = 17,
        kCloseCurly = 18,
        kOpenSquare = 19,
        kCloseSquare =  20,
        kComma = 21,
        kSemicolon = 22,
        kColon = 23,
        kCaret = 24,
        kTilde = 25,
        kHash = 26,
        kGrave = 27,
        kVar = 28,
        kArg = 29,
        kConst = 30,
        kClassVar = 31,
        kIdentifier = 32,
        kClassName = 33,
        kDot = 34,
        kDotDot = 35,
        kEllipses = 36,
        kCurryArgument = 37,

        // Control Flow
        kIf = 38
    };

    // Both are zero-based.
    struct Location {
        size_t lineNumber;
        size_t characterNumber;
    };

    Name name;
    std::string_view range;
    Location location;
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

#endif // SRC_COMPILER_INCLUDE_HADRON_TOKEN_HPP_