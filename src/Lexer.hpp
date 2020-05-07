#ifndef SRC_LEXER_HPP_
#define SRC_LEXER_HPP_

#include <string_view>
#include <vector>

namespace hadron {

class Lexer {
public:
    struct Token {
        enum Type {
            kName,
            kInteger,
            kFloat,   // SC_FLOAT
            kAccidental,
            kSymbol,
            kString,
            kCharacter,  // ASCII
            kPrimitiveName,
            kClassName,
            kCurryArg,
            kVar,  // VAR
            kClassVar,
            kConst, // SC_CONST
            kNil, // NILOBJ
            kTrue,  // TRUEOBJ
            kFalse, // FALSEOBJ
            kPseudoVar,
            kEllipsis,
            kDotDot, // ??
            kPie, // ??
            kBeginClosedFunc,
            kBadToken,
            kInterpret,
            kBeginGenerator,
            kLeftArrow,
            kWhile,
            kReadWriteVar,
            kKeyBinOp,
            kBinOp,
            kUMinus
        };

        Type type;

        /*! Start position of the token. Offset in bytes from the start of the string. */
        size_t start;

        /*! Length of the token in bytes. */
        size_t length;
    };

    Lexer(std::string_view code);
    bool lex();

    const std::vector<Token>& tokens() { return m_tokens; }

private:
    std::string_view m_code;
    std::vector<Token> m_tokens;
};

}

#endif // SRC_LEXER_HPP_
