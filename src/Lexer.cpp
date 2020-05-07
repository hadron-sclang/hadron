#include "Lexer.hpp"

// Design following https://nothings.org/computer/lexing.html

namespace {

enum State : uint8_t {
    kSawZero = 0,   // non-final
    kSawDigit = 1,  // final
    kLoneZero = 2,  // final
    kFloatStart = 3 // final
};

// premultiply?
enum Class : uint8_t {
    kWhitespace = 0,  // space, tab
    kNewline,         // \n
    kReturn,          // \r
    kZero,            // 0
    kDigit,           // 1-9
    kPeriod,          // .
    kInvalid,         // unsupported character
    kEndOfInput       // EOF, \0
};


const uint8_t kCharacterClasses[256] = {
    kEndOfInput /*   0 \0  */, kInvalid    /*   1 SOH */, kInvalid /*   2 STX */, kInvalid /*   3 ETX */,
    kInvalid    /*   4 EOT */, kInvalid    /*   5 ENQ */, kInvalid /*   6 ACK */, kInvalid /*   7 BEL */,
    kInvalid    /*   8 BS  */, kWhitespace /*   9 \t  */, kNewline /*  10 \n  */, kInvalid /*  11 VT  */,
    kInvalid    /*  12 FF  */, kReturn     /*  13 \r  */, kInvalid /*  14 SO  */, kInvalid /*  15 SI  */,
    kInvalid    /*  16 DLE */, kInvalid    /*  17 DC1 */, kInvalid /*  18 DC2 */, kInvalid /*  19 DC3 */,
    kInvalid    /*  20 DC4 */, kInvalid    /*  21 NAK */, kInvalid /*  22 SYN */, kInvalid /*  23 ETB */,
    kInvalid    /*  24 CAN */, kInvalid    /*  25 EM  */, kInvalid /*  26 SUB */, kInvalid /*  27 ESC */,
    kInvalid    /*  28 FS  */, kInvalid    /*  29 FS  */, kInvalid /*  30 RS  */, kInvalid /*  31 US  */,
    kWhitespace /*  32 SPC */, kInvalid    /*  33 !   */, kInvalid /*  34 "   */, kInvalid /*  35 #   */,
    kInvalid    /*  36 $   */, kInvalid    /*  37 %   */, kInvalid /*  38 &   */, kInvalid /*  39 '   */,
    kInvalid    /*  40 (   */, kInvalid    /*  41 )   */, kInvalid /*  42 *   */, kInvalid /*  43 +   */,
    kInvalid    /*  44 ,   */, kInvalid    /*  45 -   */, kInvalid /*  46 .   */, kInvalid /*  47 /   */,
    kZero       /*  48 0   */, kDigit      /*  49 1   */, kDigit   /*  50 2   */, kDigit   /*  51 3   */,
    kDigit      /*  52 4   */, kDigit      /*  53 5   */, kDigit   /*  54 6   */, kDigit   /*  55 7   */,
    kDigit      /*  56 8   */, kDigit      /*  57 9   */, kInvalid /*  58 :   */, kInvalid /*  59 ;   */,
    kInvalid    /*  60 <   */, kInvalid    /*  61 =   */, kInvalid /*  62 >   */, kInvalid /*  63 ?   */,
    kInvalid    /*  64 @   */, kInvalid    /*  65 A   */, kInvalid /*  66 B   */, kInvalid /*  67 C   */,
};

}

namespace hadron {

Lexer::Lexer(std::string_view code): m_code(code) {}

bool Lexer::lex() {


    return true;
}

}
