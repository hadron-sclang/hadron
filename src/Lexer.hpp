#ifndef SRC_LEXER_HPP_
#define SRC_LEXER_HPP_

#include <stdint.h>
#include <vector>

namespace hadron {

class Lexer {
public:
    struct Token {
        enum Type {
            // kName,
            kInteger,
            kFloat,
            // kAccidental,
            // kSymbol,
            // kString,
            // kCharacter,  // ASCII
            // kPrimitiveName,
            // kClassName,
            // kCurryArg,
            // kVar,  // VAR
            // kClassVar,
            // kConst, // SC_CONST
            // kNil, // NILOBJ
            // kTrue,  // TRUEOBJ
            // kFalse, // FALSEOBJ
            // kPseudoVar,
            // kEllipsis,
            // kDotDot, // ??
            // kPie, // ??
            // kBeginClosedFunc,
            // kBadToken,
            // kInterpret,
            // kBeginGenerator,
            // kLeftArrow,
            // kWhile,
            // kReadWriteVar,
            // kKeyBinOp,
            // kBinOp,
            // kUMinus
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

        /*! Makes a kInteger token */
        Token(const char* s, size_t l, int64_t intValue): type(kInteger), start(s), length(l), value(intValue) {}
    };

    Lexer(const char* code);
    bool lex();

    const std::vector<Token>& tokens() { return m_tokens; }

private:
    const char* m_code;
    std::vector<Token> m_tokens;
};

}

#endif // SRC_LEXER_HPP_
