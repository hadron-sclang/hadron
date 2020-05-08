#include "Lexer.hpp"

#include <cstdlib>

// Design following https://nothings.org/computer/lexing.html

namespace {

    // Point of state table is to minimize branching. So we only select final states when either we're in a corner
    // case that will require exceptional processing (radix numbers) or things are clear what we're parsing (floats)
enum State : uint8_t {
    // non-final states
    kBeginToken = 0,
    kInitialZero = 1,
    kNumber = 2,  // can still have radix or decimal TODO: can do flats/sharps as a separate token?

    kFirstFinalState = 3,

    // final states
    kInteger = 3,
    kHexInteger = 4, // done, this can only be an integer
    kFloat = 5, // done, we have enough to delegate to strtod
    kRadix = 6,  // can have decimal but all bets on speed are off.
    kLoneZero = 7,

    kLexError = 8,
    kEndCode = 9
};

// TODO: premultiply
enum CharacterClass : uint8_t {
    kWhitespace = 0,  // space, tab
    kNewline = 3,     // \n
    kReturn = 6,      // \r
    kZero = 9,        // 0
    kDigit = 12,      // 1-9
    kPeriod = 15,     // .
    kLowerX = 18,     // x possibly for hexadecimal
    kEndOfInput = 21, // EOF, \0
    kInvalid = 24,    // unsupported character
};

const State kStateTransitionTable[] = {
    // **** CharacterClass = kWhitespace
    /* kBeginToken  => */ kBeginToken,
    /* kInitialZero => */ kLoneZero,
    /* kNumber      => */ kInteger,

    // Class = kNewline
    /* kBeginToken  => */ kBeginToken,
    /* kInitialZero => */ kLoneZero,
    /* kNumber      => */ kInteger,

    // Class = kReturn
    /* kBeginToken  => */ kBeginToken,
    /* kInitialZero => */ kLoneZero,
    /* kNumber      => */ kInteger,

    // Class = kZero
    /* kBeginToken  => */ kInitialZero,
    /* kInitialZero => */ kInitialZero,  // many leading zeros treated as a single leading zero in sclang
    /* kNumber      => */ kNumber,

    // Class = kDigit
    /* kBeginToken  => */ kNumber,
    /* kInitialZero => */ kNumber,
    /* kNumber      => */ kNumber,

    // Class = kPeriod
    /* kBeginToken  => */ kLexError,
    /* kInitialZero => */ kFloat,
    /* kNumber      => */ kFloat,

    // Class = kLowerX
    /* kBeginToken  => */ kLexError,
    /* kInitialZero => */ kHexInteger,
    /* kNumber      => */ kLexError,

    // Class = kEndOfInput
    /* kBeginToken  => */ kEndCode,
    /* kInitialZero => */ kLoneZero,
    /* kNumber      => */ kInteger,

    // Class = kInvalid
    /* kBeginToken  => */ kEndCode,
    /* kInitialZero => */ kLexError,
    /* kNumber      => */ kLexError
};

const CharacterClass kCharacterClasses[256] = {
    kEndOfInput /*   0 \0  */, kInvalid    /*   1 SOH */, kInvalid /*   2 STX */, kInvalid /*   3 ETX */,
    kInvalid    /*   4 EOT */, kEndOfInput /*   5 EOF */, kInvalid /*   6 ACK */, kInvalid /*   7 BEL */,
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
    kLowerX     /* 120 x   */, kInvalid    /* 121 y   */, kInvalid /* 122 z   */, kInvalid /* 123 {   */,
    kInvalid    /* 124 |   */, kInvalid    /* 125 }   */, kInvalid /* 126 ~   */, kInvalid /* 127 DEL */,
    kInvalid    /* 128     */, kInvalid    /* 129     */, kInvalid /* 130     */, kInvalid /* 131     */,
    kInvalid    /* 132     */, kInvalid    /* 133     */, kInvalid /* 134     */, kInvalid /* 135     */,
    kInvalid    /* 136     */, kInvalid    /* 137     */, kInvalid /* 138     */, kInvalid /* 139     */,
    kInvalid    /* 140     */, kInvalid    /* 141     */, kInvalid /* 142     */, kInvalid /* 143     */,
    kInvalid    /* 144     */, kInvalid    /* 145     */, kInvalid /* 146     */, kInvalid /* 147     */,
    kInvalid    /* 148     */, kInvalid    /* 149     */, kInvalid /* 150     */, kInvalid /* 151     */,
    kInvalid    /* 152     */, kInvalid    /* 153     */, kInvalid /* 154     */, kInvalid /* 155     */,
    kInvalid    /* 156     */, kInvalid    /* 157     */, kInvalid /* 158     */, kInvalid /* 159     */,
    kInvalid    /* 160     */, kInvalid    /* 161     */, kInvalid /* 162     */, kInvalid /* 163     */,
    kInvalid    /* 164     */, kInvalid    /* 165     */, kInvalid /* 166     */, kInvalid /* 167     */,
    kInvalid    /* 168     */, kInvalid    /* 169     */, kInvalid /* 170     */, kInvalid /* 171     */,
    kInvalid    /* 172     */, kInvalid    /* 173     */, kInvalid /* 174     */, kInvalid /* 175     */,
    kInvalid    /* 176     */, kInvalid    /* 177     */, kInvalid /* 178     */, kInvalid /* 179     */,
    kInvalid    /* 180     */, kInvalid    /* 181     */, kInvalid /* 182     */, kInvalid /* 183     */,
    kInvalid    /* 184     */, kInvalid    /* 185     */, kInvalid /* 186     */, kInvalid /* 187     */,
    kInvalid    /* 188     */, kInvalid    /* 189     */, kInvalid /* 190     */, kInvalid /* 191     */,
    kInvalid    /* 192     */, kInvalid    /* 193     */, kInvalid /* 194     */, kInvalid /* 195     */,
    kInvalid    /* 196     */, kInvalid    /* 197     */, kInvalid /* 198     */, kInvalid /* 199     */,
    kInvalid    /* 200     */, kInvalid    /* 201     */, kInvalid /* 202     */, kInvalid /* 203     */,
    kInvalid    /* 204     */, kInvalid    /* 205     */, kInvalid /* 206     */, kInvalid /* 207     */,
    kInvalid    /* 208     */, kInvalid    /* 209     */, kInvalid /* 210     */, kInvalid /* 211     */,
    kInvalid    /* 212     */, kInvalid    /* 213     */, kInvalid /* 214     */, kInvalid /* 215     */,
    kInvalid    /* 216     */, kInvalid    /* 217     */, kInvalid /* 218     */, kInvalid /* 219     */,
    kInvalid    /* 220     */, kInvalid    /* 221     */, kInvalid /* 222     */, kInvalid /* 223     */,
    kInvalid    /* 224     */, kInvalid    /* 225     */, kInvalid /* 226     */, kInvalid /* 227     */,
    kInvalid    /* 228     */, kInvalid    /* 229     */, kInvalid /* 230     */, kInvalid /* 231     */,
    kInvalid    /* 232     */, kInvalid    /* 233     */, kInvalid /* 234     */, kInvalid /* 235     */,
    kInvalid    /* 236     */, kInvalid    /* 237     */, kInvalid /* 238     */, kInvalid /* 239     */,
    kInvalid    /* 240     */, kInvalid    /* 241     */, kInvalid /* 242     */, kInvalid /* 243     */,
    kInvalid    /* 244     */, kInvalid    /* 245     */, kInvalid /* 246     */, kInvalid /* 247     */,
    kInvalid    /* 248     */, kInvalid    /* 249     */, kInvalid /* 250     */, kInvalid /* 251     */,
    kInvalid    /* 252     */, kInvalid    /* 253     */, kInvalid /* 254     */, kInvalid /* 255     */,
};

int8_t kStateLengths[] = {
    0, // kBeginToken
    1, // kInitialZero
    1, // kNumber
    0, // kInteger
    0, // kHexInteger
    0, // kFloat
    0, // kRadix
    0, // kLoneZero
    0, // kLexError
    0, // kEndCode
};

}

namespace hadron {

Lexer::Lexer(const char* code): m_code(code) {}

bool Lexer::lex() {
    const char* code = m_code;
    bool lexContinues = true;
    State state;
    while (lexContinues) {
        int tokenLength = 0;
        state = kBeginToken;
        do {
            int character = static_cast<int>(*code++);
            int characterClass = static_cast<int>(kCharacterClasses[character]);
            state = kStateTransitionTable[characterClass + state];
            tokenLength += kStateLengths[state];
        } while (state < State::kFirstFinalState);

        char* tokenEnd = nullptr;
        const char* tokenStart;
        switch(state) {
            case kInteger: {
                // Exit state machine pointing just past the end of the integer.
                tokenStart = code - tokenLength - 1;
                int64_t intValue = std::strtoll(tokenStart, &tokenEnd, 10);
                if (tokenStart < tokenEnd) {
                    m_tokens.emplace_back(Token(tokenStart, tokenEnd - tokenStart, intValue));
                } else {
                    lexContinues = false;
                }
            } break;

            case kHexInteger: {
                // Exit state machine pointing just past the "0x"
                tokenStart = code - tokenLength - 1;
                int64_t intValue = std::strtoll(code, &tokenEnd, 16);
                if (code < tokenEnd) {
                    m_tokens.emplace_back(Token(tokenStart, tokenEnd - tokenStart, intValue));
                    code = tokenEnd + 1;
                } else {
                    lexContinues = false;
                }
            } break;

            case kFloat:
            case kRadix:

            case kLoneZero:
                tokenStart = code - tokenLength - 1;
                m_tokens.emplace_back(Token(tokenStart, tokenLength, 0LL));
                break;

            case kLexError:
            case kEndCode:
                lexContinues = false;
                break;
        }
    }

    return state == kEndCode;
}

}
