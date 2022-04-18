#ifndef SRC_COMPILER_INCLUDE_HADRON_TOKEN_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_TOKEN_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"

#include <string_view>

namespace hadron {

enum LiteralType {
    kNone,
    kNil,
    kInteger,
    kFloat,
    kBoolean,
    kChar,
    kBlock, // to go separete
    kSymbol // to go separate for copies
};

// Lexer lexes source to produce Tokens, Parser consumes Tokens to produce Parse Tree.
struct Token {
    Token() = delete;
    ~Token() = default;

    enum Name {
        // ** Modifying these number values? Please make sure to update server/JSONTransport.cpp too!
        kEmpty = 0,     // represents no token
        kInterpret = 1, // The LSC grammar is ambiguous as written without the insertion of a special token informing
                        // the parser that the input text is interpreted code. Without this the grammar cannot determine
                        // if a classname input is a class definition or a reference to a class as part of an
                        // expression. To fix we inject this token at the beginning of interpreted code. There may be
                        // other ways to resolve this ambiguity but they will likely require some changes to the
                        // grammar.
        kLiteral = 2,
        kString = 3,    // Strings are lexed differently from other literals to allow support for concatenating literal
                        // strings at compile time, e.g. "line1" "line2" "line3" should end up as one string in the AST.
        kPrimitive = 4,

        // <<< all below could also be binops >>>
        kPlus = 5,         // so named because it could be an addition or a class extension
        kMinus = 6,        // Could be unary negation so handled separately
        kAsterisk = 7,     // so named because it could be a multiply or a class method
        kAssign = 8,
        kLessThan = 9,
        kGreaterThan = 10,
        kPipe = 11,
        kReadWriteVar = 12,
        kLeftArrow = 13,
        kBinop = 14,  // TODO: rename kGenericBinop, this is some arbitrary collection of the valid binop characters.
        kKeyword = 15,      // Any identifier with a colon after it.
        // <<< all above could also be binops >>>

        kOpenParen = 16,
        kCloseParen = 17,
        kOpenCurly = 18,
        kCloseCurly = 19,
        kOpenSquare = 20,
        kCloseSquare = 21,
        kComma = 22,
        kSemicolon = 23,
        kColon = 24,
        kCaret = 25,
        kTilde = 26,
        kHash = 27,
        kGrave = 28,
        kVar = 29,
        kArg = 30,
        kConst = 31,
        kClassVar = 32,
        kIdentifier = 33,
        kClassName = 34,
        kDot = 35,
        kDotDot = 36,
        kEllipses = 37,
        kCurryArgument = 38,

        // Control Flow
        kIf = 39,
        kWhile = 40
    };

    Name name;
    std::string_view range;
    Slot value;
    LiteralType literalType;
    bool couldBeBinop;
    Hash hash;
    bool escapeString;
    // Both are zero-based.
    struct Location {
        size_t lineNumber = 0;
        size_t characterNumber = 0;
    };
    Location location;

    // Method for making any non-literal token.
    static inline Token make(Name n, std::string_view r, Location loc, bool binop = false, Hash h = 0) {
        return Token(n, r, Slot::makeNil(), LiteralType::kNone, binop, h, false, loc);
    }
    static inline Token makeIntegerLiteral(int32_t intValue, std::string_view r, Location loc) {
        return Token(kLiteral, r, Slot::makeInt32(intValue), LiteralType::kInteger, false, 0, false, loc);
    }
    static inline Token makeFloatLiteral(double f, std::string_view r, Location loc) {
        return Token(kLiteral, r, Slot::makeFloat(f), LiteralType::kFloat, false, 0, false, loc);
    }
    // Note we don't copy strings or symbols into SC-side String or Symbol objects for now
    static inline Token makeString(std::string_view r, Location loc, bool escape) {
        return Token(kString, r, Slot::makeNil(), LiteralType::kNone, false, 0, escape, loc);
    }
    static inline Token makeSymbolLiteral(std::string_view r, Location loc, bool escape) {
        return Token(kLiteral, r, Slot::makeNil(), LiteralType::kSymbol, false, 0, escape, loc);
    }
    static inline Token makeCharLiteral(char c, std::string_view r, Location loc) {
        return Token(kLiteral, r, Slot::makeChar(c), LiteralType::kChar, false, 0, false, loc);
    }
    static inline Token makeBooleanLiteral(bool b, std::string_view r, Location loc) {
        return Token(kLiteral, r, Slot::makeBool(b), LiteralType::kBoolean, false, 0, false, loc);
    }
    static inline Token makeNilLiteral(std::string_view r, Location loc) {
        return Token(kLiteral, r, Slot::makeNil(), LiteralType::kNil, false, 0, false, loc);
    }
    static inline Token makeEmpty() {
        return Token(kEmpty, std::string_view(), Slot::makeNil(), LiteralType::kNone, false, 0, false, Location{0, 0});
    }

private:
    Token(Name n, std::string_view r, Slot v, LiteralType t, bool binop, Hash h, bool escape, Location l):
        name(n), range(r), value(v), literalType(t), couldBeBinop(binop), hash(h), escapeString(escape), location(l) {}
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_TOKEN_HPP_