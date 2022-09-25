#ifndef SRC_COMPILER_INCLUDE_HADRON_TOKEN_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_TOKEN_HPP_

#include "hadron/Slot.hpp"

#include <string_view>

namespace hadron {

// Lexer lexes source to produce Tokens, Parser consumes Tokens to produce Parse Tree.
struct Token {
    Token() = delete;
    ~Token() = default;

    enum Name {
        kEmpty = 0, // represents no token
        kInterpret = 1, // The LSC grammar is ambiguous as written without the insertion of a special token informing
                        // the parser that the input text is interpreted code. Without this the grammar cannot determine
                        // if a classname input is a class definition or a reference to a class as part of an
                        // expression. To fix we inject this token at the beginning of interpreted code. There may be
                        // other ways to resolve this ambiguity but they will likely require some changes to the
                        // grammar.
        kLiteral = 2,
        kAccidental = 3,
        kPi = 4,
        kString = 5, // Strings are lexed differently from other literals to allow support for concatenating literal
                     // strings at compile time, e.g. "line1" "line2" "line3" should end up as one string in the AST.
        kSymbol = 6,
        kPrimitive = 7,

        // <<< all below could also be binops >>>
        kPlus = 8, // so named because it could be an addition or a class extension
        kMinus = 9, // Could be unary negation so handled separately
        kAsterisk = 10, // so named because it could be a multiply or a class method
        kAssign = 11,
        kLessThan = 12,
        kGreaterThan = 13,
        kPipe = 14,
        kReadWriteVar = 15,
        kLeftArrow = 16,
        kBinop = 17, // TODO: rename kGenericBinop, this is some arbitrary collection of the valid binop characters.
        kKeyword = 18, // Any identifier with a colon after it.
        // <<< all above could also be binops >>>

        kOpenParen = 19,
        kCloseParen = 20,
        kOpenCurly = 21,
        kCloseCurly = 22,
        kOpenSquare = 23,
        kCloseSquare = 24,
        kComma = 25,
        kSemicolon = 26,
        kColon = 27,
        kCaret = 28,
        kTilde = 29,
        kHash = 30,
        kGrave = 31,
        kVar = 32,
        kArg = 33,
        kConst = 34,
        kClassVar = 35,
        kIdentifier = 36,
        kClassName = 37,
        kDot = 38,
        kDotDot = 39,
        kEllipses = 40,
        kCurryArgument = 41,
        kBeginClosedFunction = 42, // #{

        // Control Flow
        kIf = 43,
        kWhile = 44
    };

    Name name;
    std::string_view range;
    Slot value;
    bool couldBeBinop;
    bool escapeString;
    // Both are zero-based.
    struct Location {
        int32_t lineNumber = 0;
        int32_t characterNumber = 0;
    };
    Location location;

    // Method for making any non-literal token.
    static inline Token make(Name n, std::string_view r, Location loc, bool binop = false) {
        return Token(n, r, Slot::makeNil(), binop, false, loc);
    }
    static inline Token makeIntegerLiteral(int32_t intValue, std::string_view r, Location loc) {
        return Token(kLiteral, r, Slot::makeInt32(intValue), false, false, loc);
    }
    static inline Token makeFloatLiteral(double f, std::string_view r, Location loc) {
        return Token(kLiteral, r, Slot::makeFloat(f), false, false, loc);
    }
    static inline Token makeAccidentalLiteral(double f, std::string_view r, Location loc) {
        return Token(kAccidental, r, Slot::makeFloat(f), false, false, loc);
    }
    static inline Token makePi(std::string_view r, Location loc) {
        return Token(kPi, r, Slot::makeNil(), false, false, loc);
    }
    // Note we don't copy strings or symbols into SC-side String or Symbol objects for now
    static inline Token makeString(std::string_view r, Location loc, bool escape) {
        return Token(kString, r, Slot::makeNil(), false, escape, loc);
    }
    static inline Token makeSymbol(std::string_view r, Location loc, bool escape) {
        return Token(kSymbol, r, Slot::makeNil(), false, escape, loc);
    }
    static inline Token makeCharLiteral(char c, std::string_view r, Location loc) {
        return Token(kLiteral, r, Slot::makeChar(c), false, false, loc);
    }
    static inline Token makeBooleanLiteral(bool b, std::string_view r, Location loc) {
        return Token(kLiteral, r, Slot::makeBool(b), false, false, loc);
    }
    static inline Token makeNilLiteral(std::string_view r, Location loc) {
        return Token(kLiteral, r, Slot::makeNil(), false, false, loc);
    }
    static inline Token makeEmpty() {
        return Token(kEmpty, std::string_view(), Slot::makeNil(), false, false, Location { 0, 0 });
    }

private:
    Token(Name n, std::string_view r, Slot v, bool binop, bool escape, Location l):
        name(n), range(r), value(v), couldBeBinop(binop), escapeString(escape), location(l) { }
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_TOKEN_HPP_