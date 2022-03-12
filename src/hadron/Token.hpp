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
    kString,
    kSymbol,
    kObject,
    kArray,
    kBlock,
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
        kIf = 38,
        kWhile = 39
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
    static inline Token makeStringLiteral(std::string_view r, Location loc, bool escape) {
        return Token(kLiteral, r, Slot::makeNil(), LiteralType::kString, false, 0, escape, loc);
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