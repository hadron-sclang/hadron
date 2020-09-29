#ifndef SRC_LEXER_HPP_
#define SRC_LEXER_HPP_

#include <cstddef>
#include <stdint.h>
#include <vector>

namespace hadron {

class Lexer {
public:
    struct Token {
        enum Type {
            kEmpty,  // represents no token
            kInteger,
            kFloat,
            kString,
            kSymbol,
            kPlus,         // so named because it could be an addition or a class extension
            kAsterisk,     // so named because it could be a multiply or a class method
            kAssign,
            kLessThan,
            kGreaterThan,
            kPipe,
            kReadWriteVar,
            kLeftArrow,
            kBinop,
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
            kVar,
            kNil,
            kArg,
            kTrue,
            kFalse,
            kIdentifier,
            kClassName,
            kDot,
            kDotDot,
            kEllipses
        };

        Type type;

        /*! Start position of the token. */
        const char* start;

        /*! Length of the token in bytes. */
        size_t length;

        union Value {
            Value(): integer(0) {}
            Value(int64_t v): integer(v) {}
            Value(double v): floatingPoint(v) {}

            int64_t integer;
            double floatingPoint;
        };
        Value value;

        Token(): type(kEmpty), start(nullptr), length(0) {}

        /*! Makes a kInteger token */
        Token(const char* s, size_t l, int64_t intValue): type(kInteger), start(s), length(l), value(intValue) {}

        /*! Makes a kFloat token */
        Token(const char* s, size_t l, double doubleValue): type(kFloat), start(s), length(l), value(doubleValue) {}

        /*! Makes a token with no value storage */
        Token(Type t, const char* s, size_t l): type(t), start(s), length(l) {}
    };

    Lexer(std::string_view code);
    bool lex(

private:
    std::vector<Token> m_tokens;

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
