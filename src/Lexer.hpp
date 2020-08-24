#ifndef SRC_LEXER_HPP_
#define SRC_LEXER_HPP_

#include <stdint.h>
#include <vector>

namespace hadron {

class Lexer {
public:
    struct Token {
        enum Type {
            kInteger,
            kString,
            kSymbol,
            kAddition,
            kOpenParen,
            kCloseParen
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

        /*! Makes a token with no value storage */
        Token(Type t, const char* s, size_t l): type(t), start(s), length(l) {}
    };

    Lexer(const char* code);
    Lexer(const char* code, size_t length);

    bool lex();

    const std::vector<Token>& tokens() { return m_tokens; }

#ifdef DEBUG_LEXER
    /*! Save a dotfile of the Lexer state machine to the provided path.
     */
    static void saveLexerStateMachineGraph(const char* fileName);
#endif

private:
    const char* m_code;
    size_t m_length;
    std::vector<Token> m_tokens;
};

} // namespace hadron

#endif // SRC_LEXER_HPP_
