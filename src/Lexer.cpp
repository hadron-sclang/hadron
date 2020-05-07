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
    kInvalid    /*  68 D   */, kInvalid    /*  69 E   */, kInvalid /*  70 F   */, kInvalid /*  71 G   */,
    kInvalid    /*  72 H   */, kInvalid    /*  73 I   */, kInvalid /*  74 J   */, kInvalid /*  75 K   */,
    kInvalid    /*  76 L   */, kInvalid    /*  77 M   */, kInvalid /*  78 N   */, kInvalid /*  79 O   */,
    kInvalid    /*  80 P   */, kInvalid    /*  81 Q   */, kInvalid /*  82 R   */, kInvalid /*  83 S   */,
    kInvalid    /*  84 T   */, kInvalid    /*  85 U   */, kInvalid /*  86 V   */, kInvalid /*  87 W   */,
    kInvalid    /*  88 X   */, kInvalid    /*  89 Y   */, kInvalid /*  90 Z   */, kInvalid /*  91 [   */,
    kInvalid    /*  92 \   */, kInvalid    /*  93 ]   */, kInvalid /*  94 ^   */, kInvalid /*  95 _   */,
    kInvalid    /*  96 `   */, kInvalid    /*  97 a   */, kInvalid /*  98 b   */, kInvalid /*  99 c   */,
    kInvalid    /* 100 d   */, kInvalid    /* 101 e   */, kInvalid /* 102 f   */, kInvalid /* 103 g   */,
    kInvalid    /* 104 h   */, kInvalid    /* 105 i   */, kInvalid /* 106 j   */, kInvalid /* 107 k   */,
    kInvalid    /* 108 l   */, kInvalid    /* 109 m   */, kInvalid /* 110 n   */, kInvalid /* 111 o   */,
    kInvalid    /* 112 p   */, kInvalid    /* 113 q   */, kInvalid /* 114 r   */, kInvalid /* 115 s   */,
    kInvalid    /* 116 t   */, kInvalid    /* 117 u   */, kInvalid /* 118 v   */, kInvalid /* 119 w   */,
    kInvalid    /* 120 x   */, kInvalid    /* 121 y   */, kInvalid /* 122 z   */, kInvalid /* 123 {   */,
    kInvalid    /* 124 |   */, kInvalid    /* 125 }   */, kInvalid /* 126 ~   */, kInvalid /* 127 DEL */,
    // Remainder of characters > 127 are all invalid.
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, 
    kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid, kInvalid
};

}

namespace hadron {

Lexer::Lexer(std::string_view code): m_code(code) {}

bool Lexer::lex() {


    return true;
}

}
